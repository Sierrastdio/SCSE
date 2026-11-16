#include "engine/detail.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace spasim::detail
{
namespace
{

void applyTorpedoBlast(World &world, const Torpedo &torpedo, double radius)
{
  for (auto &slot : world.ships)
  {
    if (!slot || !slot->exists || slot->combat.destroyed || slot->owner == torpedo.owner
        || (torpedo.team < world.teams.size() && slot->team < world.teams.size()
            && std::ranges::find(world.teams[torpedo.team].allies, slot->team) != world.teams[torpedo.team].allies.end()
            && !world.friendlyFire))
    {
      continue;
    }

    const double distance = slot->position.distanceTo(torpedo.position);

    if (distance > radius)
    {
      continue;
    }

    const double damage = torpedo.damage * std::exp(-distance / radius);

    const double shieldDamage = std::min(slot->combat.shields, damage);

    slot->combat.shields -= shieldDamage;
    slot->combat.hullDamage = std::min(slot->combat.hull, slot->combat.hullDamage + (damage - shieldDamage));

    if (slot->combat.hullDamage >= slot->combat.hull)
      slot->combat.destroyed = true;
  }
}

double dot(const Vec3 &lhs, const Vec3 &rhs) noexcept { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }

void applyProjectileImpact(World &world, WeaponProjectile &projectile)
{
  if (auto *torpedo = world.findTorpedo(projectile.target))
  {
    torpedo->energy = std::max(0.0, torpedo->energy - projectile.damage);

    if (torpedo->energy == 0.0)
    {
      torpedo->exists = false;
    }

    return;
  }

  if (auto *targetCombat = combatState(world, Target{Target::Kind::Entity, projectile.target}))
  {
    const auto targetTeam = teamOf(world, projectile.target);

    if (targetTeam && *targetTeam < world.teams.size() && projectile.team < world.teams.size()
        && std::ranges::find(world.teams[projectile.team].allies, *targetTeam) != world.teams[projectile.team].allies.end()
        && !world.friendlyFire)
    {
      return;
    }

    const double shieldDamage = std::min(targetCombat->shields, projectile.damage);
    targetCombat->shields -= shieldDamage;
    targetCombat->hullDamage = std::min(targetCombat->hull, targetCombat->hullDamage + (projectile.damage - shieldDamage));

    if (targetCombat->hullDamage >= targetCombat->hull)
    {
      targetCombat->destroyed = true;
    }
  }
}

} // namespace

CommandResult executeAction(World &world, Player &player, Ship &ship, const FirePhasor &action, const GameRules &rules)
{
  if (!std::isfinite(action.power) || action.power <= 0.0)
  {
    return {CommandStatus::Invalid, "Weapon power must be positive and finite."};
  }

  if (!ship.selectedTarget)
  {
    return {CommandStatus::Invalid, "Select a target first."};
  }

  const auto weaponIndex = static_cast<std::size_t>(ship.weapon);

  if (weaponIndex >= world.weapons.size())
  {
    return {CommandStatus::Invalid, "Selected weapon profile is invalid."};
  }

  const auto &weapon = world.weapons[weaponIndex];
  const double cost = weapon.energyCost * action.power;

  if (ship.resources.energy < cost)
  {
    return {CommandStatus::Invalid, "Insufficient energy."};
  }

  const EntityId targetId = *ship.selectedTarget;
  const auto targetTeam = teamOf(world, targetId);

  if (!targetTeam)
  {
    return {CommandStatus::NotFound, "Selected target no longer exists."};
  }

  if (*targetTeam < world.teams.size()
      && std::ranges::find(world.teams[player.team].allies, *targetTeam) != world.teams[player.team].allies.end()
      && !world.friendlyFire)
  {
    return {CommandStatus::Invalid, "Cannot fire on an allied target."};
  }

  auto *targetPosition = positionOf(world, Target{Target::Kind::Entity, targetId});

  if (!targetPosition)
  {
    return {CommandStatus::Invalid, "Target is outside weapon range."};
  }

  const Vec3 direction = *targetPosition - ship.position;
  const double distance = direction.length();

  if (distance > weapon.range)
  {
    return {CommandStatus::Invalid, "Target is outside weapon range."};
  }

  if (!rules.isWithinFiringArc(ship.orientation, direction, weapon.aim))
  {
    return {CommandStatus::Invalid, "Target is outside the weapon's horizontal or vertical firing limits."};
  }

  Vec3 targetVelocity{};

  if (const auto *targetShip = world.findShip(targetId))
  {
    targetVelocity = targetShip->velocity;
  }
  else if (const auto *targetTorpedo = world.findTorpedo(targetId))
  {
    targetVelocity = targetTorpedo->velocity;
  }

  if (weapon.projectileSpeed <= 1e-9)
  {
    ship.resources.energy -= cost;
    WeaponProjectile hit{.target = targetId, .team = ship.team, .damage = weapon.damage * action.power};
    applyProjectileImpact(world, hit);
    return {CommandStatus::Applied, "Target hit."};
  }

  const Vec3 aim = rules.weaponAimDirection(direction, targetVelocity - ship.velocity, weapon.projectileSpeed, weapon.aim);
  ship.resources.energy -= cost;

  world.weaponProjectiles.push_back({.id = world.nextEntityId++,
    .shooter = ship.id,
    .target = targetId,
    .team = ship.team,
    .position = ship.position,
    .velocity = ship.velocity + aim * weapon.projectileSpeed,
    .speed = weapon.projectileSpeed,
    .damage = weapon.damage * action.power,
    .collisionRadius = std::max(player.phasorDiameter * 0.5, 0.1),
    .remainingRange = weapon.range,
    .explosive = weapon.explosive});
  return {CommandStatus::Applied, "Weapon projectile launched."};
}

CommandResult executeAction(World &world, Player &player, Ship &ship, const FireTorpedo &action)
{
  if (!std::isfinite(action.energy) || action.energy <= 0.0 || ship.resources.energy < action.energy)
  {
    return {CommandStatus::Invalid, "Invalid torpedo energy or insufficient energy."};
  }

  auto *torpedoSlot = world.findTorpedo(ship.torpedoId);

  if (!torpedoSlot)
  {
    return {CommandStatus::NotFound, "Your torpedo slot is missing."};
  }

  if (torpedoSlot->exists)
  {
    return {CommandStatus::Invalid, "Your torpedo is already active."};
  }

  Vec3 aim = action.useShipFacing ? forwardVector(ship.orientation) : action.aim;

  if (ship.selectedTarget)
  {
    const Vec3 *targetPosition = positionOf(world, Target{Target::Kind::Entity, *ship.selectedTarget});
    if (targetPosition)
    {
      aim = *targetPosition - ship.position;
    }
  }

  const double aimLength = aim.length();

  if (!std::isfinite(aimLength) || aimLength <= 1e-9)
  {
    return {CommandStatus::Invalid, "Torpedo aim vector must be finite and non-zero."};
  }

  ship.resources.energy -= action.energy;
  aim = aim * (1.0 / aimLength);
  Torpedo torpedo;
  torpedo.id = ship.torpedoId;
  torpedo.owner = player.id;
  torpedo.team = player.team;
  torpedo.position = ship.position;
  torpedo.velocity = aim * world.torpedoLaunchSpeed;
  torpedo.energy = action.energy;
  torpedo.fieldStrength = action.energy;
  torpedo.damage = action.energy;
  torpedo.lifeSeconds = world.torpedoLifeSeconds;
  torpedo.collisionRadius = world.torpedoCollisionRadius;
  torpedo.exists = true;
  *torpedoSlot = torpedo;
  player.selectedObject = ship.torpedoId;

  return {CommandStatus::Applied, "Torpedo launched."};
}

CommandResult executeAction(World &world, Player &, Ship &ship, const DetonateTorpedo &)
{
  auto *torpedo = world.findTorpedo(ship.torpedoId);

  if (!torpedo || !torpedo->exists)
  {
    return {CommandStatus::Invalid, "No active torpedo."};
  }

  applyTorpedoBlast(world, *torpedo, 250.0);
  torpedo->exists = false;
  torpedo->detonated = true;

  return {CommandStatus::Applied, "Torpedo detonated."};
}

CommandResult executeAction(World &world, Player &player, Ship &ship, const SetWeaponProfile &action)
{
  if (action.weapon >= world.weapons.size())
  {
    return {CommandStatus::Invalid, "Weapon profile is out of range."};
  }

  player.weapon = action.weapon;
  ship.weapon = action.weapon;

  return {CommandStatus::Applied, "Weapon selected."};
}

void updateTorpedoes(World &world)
{
  for (auto &slot : world.torpedoes)
  {
    if (!slot || !slot->exists || slot->detonated)
    {
      continue;
    }

    const double dt = world.fixedStepSeconds;
    slot->velocity += gravityAcceleration(world, slot->position) * dt;
    slot->position += slot->velocity * dt;
    slot->lifeSeconds -= dt;

    const bool hitPlanet = std::ranges::any_of(world.planets, [&](const Planet &planet) {
      const Vec3 offset = slot->position - planet.position;
      const double radius = planet.radius + slot->collisionRadius;
      return offset.lengthSquared() <= radius * radius;
    });

    if (hitPlanet)
    {
      slot->exists = false;
      slot->detonated = true;
      continue;
    }

    bool impact = false;

    for (const auto &targetSlot : world.ships)
    {
      if (!targetSlot || !targetSlot->exists || targetSlot->combat.destroyed || targetSlot->owner == slot->owner
          || (slot->team < world.teams.size() && targetSlot->team < world.teams.size()
              && std::ranges::find(world.teams[slot->team].allies, targetSlot->team) != world.teams[slot->team].allies.end()
              && !world.friendlyFire))
        continue;

      const Vec3 offset = slot->position - targetSlot->position;
      const double radius = slot->collisionRadius + targetSlot->collisionRadius;

      if (offset.lengthSquared() <= radius * radius)
      {
        impact = true;
        break;
      }
    }

    if (!impact && slot->lifeSeconds > 0.0)
    {
      continue;
    }

    applyTorpedoBlast(world, *slot, impact ? 250.0 : 100.0);
    slot->exists = false;
    slot->detonated = true;
  }
}

void updateWeaponProjectiles(World &world)
{
  const double dt = world.fixedStepSeconds;
  std::erase_if(world.weaponProjectiles, [&](WeaponProjectile &projectile) {
    const Vec3 *targetPosition = positionOf(world, Target{Target::Kind::Entity, projectile.target});

    if (!targetPosition || dt <= 0.0 || projectile.remainingRange <= 0.0 || projectile.speed <= 1e-9)
    {
      return true;
    }

    Vec3 targetVelocity{};
    double targetRadius{};

    if (const auto *targetShip = world.findShip(projectile.target))
    {
      if (!targetShip->exists)
      {
        return true;
      }

      targetVelocity = targetShip->velocity;
      targetRadius = targetShip->collisionRadius;
    }
    else if (const auto *targetBase = world.findBase(projectile.target))
    {
      if (!targetBase->exists)
      {
        return true;
      }

      targetRadius = targetBase->collisionRadius;
    }
    else if (const auto *targetTorpedo = world.findTorpedo(projectile.target))
    {
      if (!targetTorpedo->exists)
      {
        return true;
      }

      targetVelocity = targetTorpedo->velocity;
      targetRadius = targetTorpedo->collisionRadius;
    }
    else
    {
      return true;
    }

    const double stepSeconds = std::min(dt, projectile.remainingRange / projectile.speed);
    const Vec3 projectileStep = projectile.velocity * stepSeconds;
    const Vec3 relativeStart = projectile.position - *targetPosition;
    const Vec3 relativeStep = projectileStep - targetVelocity * stepSeconds;
    const double relativeStepSquared = relativeStep.lengthSquared();
    const double hitFraction = relativeStepSquared > 1e-12
      ? std::clamp(-dot(relativeStart, relativeStep) / relativeStepSquared, 0.0, 1.0)
      : 0.0;
    const Vec3 closestOffset = relativeStart + relativeStep * hitFraction;
    const double hitRadius = projectile.collisionRadius + targetRadius;

    if (closestOffset.lengthSquared() <= hitRadius * hitRadius)
    {
      applyProjectileImpact(world, projectile);
      return true;
    }

    projectile.position += projectileStep;
    projectile.remainingRange -= projectile.speed * stepSeconds;

    return projectile.remainingRange <= 0.0;
  });
}

} // namespace spasim::detail
