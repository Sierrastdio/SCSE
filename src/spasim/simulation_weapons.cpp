#include "spasim/simulation_detail.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace spasim::detail {
namespace {

void applyTorpedoBlast(World& world, const Torpedo& torpedo, double radius) {
    for (auto& slot : world.ships) {
        if (!slot || !slot->exists || slot->combat.destroyed || slot->owner == torpedo.owner ||
            (torpedo.team < kTeamCount && slot->team < kTeamCount &&
             world.teams[torpedo.team].allied[slot->team] && !world.friendlyFire)) continue;

        const double distance = slot->position.distanceTo(torpedo.position);
        if (distance > radius) continue;

        const double damage = torpedo.damage * std::exp(-distance / radius);
        const double shieldDamage = std::min(slot->combat.shields, damage);
        slot->combat.shields -= shieldDamage;
        slot->combat.hullDamage = std::min(slot->combat.hull,
            slot->combat.hullDamage + (damage - shieldDamage));
        if (slot->combat.hullDamage >= slot->combat.hull) slot->combat.destroyed = true;
    }
}

} // namespace

CommandResult executeAction(World& world, Player& player, Ship& ship, const FirePhasor& action) {
    if (!std::isfinite(action.power) || action.power <= 0.0)
        return {CommandStatus::Invalid, "Weapon power must be positive and finite."};
    if (!player.selectedTarget) return {CommandStatus::Invalid, "Select a target first."};

    const auto weaponIndex = static_cast<std::size_t>(player.weapon);
    if (weaponIndex >= world.weapons.size())
        return {CommandStatus::Invalid, "Selected weapon profile is invalid."};
    const auto& weapon = world.weapons[weaponIndex];
    const double cost = weapon.energyCost * action.power;
    if (ship.resources.energy < cost)
        return {CommandStatus::Invalid, "Insufficient energy."};

    const EntityId targetId = *player.selectedTarget;
    const auto targetTeam = teamOf(world, targetId);
    if (!targetTeam) return {CommandStatus::NotFound, "Selected target no longer exists."};
    if (*targetTeam < kTeamCount && world.teams[player.team].allied[*targetTeam] && !world.friendlyFire)
        return {CommandStatus::Invalid, "Cannot fire on an allied target."};

    auto* targetPosition = positionOf(world, Target{Target::Kind::Entity, targetId});
    if (!targetPosition || ship.position.distanceTo(*targetPosition) > weapon.range)
        return {CommandStatus::Invalid, "Target is outside weapon range."};

    const Vec3 direction = *targetPosition - ship.position;
    const double distance = direction.length();
    const Vec3 forward = forwardVector(ship.orientation);
    const double alignment = (forward.x * direction.x + forward.y * direction.y + forward.z * direction.z) /
                             std::max(distance, 1e-9);
    if (alignment < 0.985)
        return {CommandStatus::Invalid, "Target is outside the weapon's forward arc."};

    if (auto* torpedo = world.findTorpedo(targetId)) {
        if (distance < player.phasorDiameter + 5.0) {
            ship.resources.energy -= cost;
            torpedo->energy = std::max(0.0, torpedo->energy - action.power);
            if (torpedo->energy == 0.0) torpedo->exists = false;
            return {CommandStatus::Applied, "Torpedo intercepted."};
        }
    }

    auto* targetCombat = combatState(world, Target{Target::Kind::Entity, targetId});
    if (!targetCombat)
        return {CommandStatus::Invalid, "Selected target cannot take weapon damage."};

    ship.resources.energy -= cost;
    const double damage = weapon.damage * action.power;
    const double shieldDamage = std::min(targetCombat->shields, damage);
    targetCombat->shields -= shieldDamage;
    targetCombat->hullDamage = std::min(targetCombat->hull,
        targetCombat->hullDamage + (damage - shieldDamage));
    if (targetCombat->hullDamage >= targetCombat->hull) targetCombat->destroyed = true;
    return {CommandStatus::Applied, targetCombat->destroyed ? "Target destroyed." : "Target hit."};
}

CommandResult executeAction(World& world, Player& player, Ship& ship, const FireTorpedo& action) {
    if (!std::isfinite(action.energy) || action.energy <= 0.0 || ship.resources.energy < action.energy)
        return {CommandStatus::Invalid, "Invalid torpedo energy or insufficient energy."};

    auto* torpedoSlot = world.findTorpedo(player.torpedoId);
    if (!torpedoSlot) return {CommandStatus::NotFound, "Your torpedo slot is missing."};
    if (torpedoSlot->exists) return {CommandStatus::Invalid, "Your torpedo is already active."};

    const double aimLength = action.aim.length();
    if (!std::isfinite(aimLength) || aimLength <= 1e-9)
        return {CommandStatus::Invalid, "Torpedo aim vector must be finite and non-zero."};

    ship.resources.energy -= action.energy;
    const Vec3 aim = action.aim * (1.0 / aimLength);
    Torpedo torpedo;
    torpedo.id = player.torpedoId;
    torpedo.owner = player.id;
    torpedo.team = player.team;
    torpedo.position = ship.position;
    torpedo.velocity = aim * 100.0;
    torpedo.energy = action.energy;
    torpedo.fieldStrength = action.energy;
    torpedo.damage = action.energy;
    torpedo.lifeSeconds = 60.0;
    torpedo.exists = true;
    *torpedoSlot = torpedo;
    player.selectedObject = player.torpedoId;
    return {CommandStatus::Applied, "Torpedo launched."};
}

CommandResult executeAction(World& world, Player& player, Ship&, const DetonateTorpedo&) {
    auto* torpedo = world.findTorpedo(player.torpedoId);
    if (!torpedo || !torpedo->exists) return {CommandStatus::Invalid, "No active torpedo."};

    applyTorpedoBlast(world, *torpedo, 250.0);
    torpedo->exists = false;
    torpedo->detonated = true;
    return {CommandStatus::Applied, "Torpedo detonated."};
}

CommandResult executeAction(World&, Player& player, Ship&, const SetWeaponProfile& action) {
    if (static_cast<std::size_t>(action.weapon) >= kWeaponCount)
        return {CommandStatus::Invalid, "Weapon profile is out of range."};
    player.weapon = action.weapon;
    return {CommandStatus::Applied, "Weapon selected."};
}

void updateTorpedoes(World& world) {
    for (auto& slot : world.torpedoes) {
        if (!slot || !slot->exists || slot->detonated) continue;

        const double dt = world.fixedStepSeconds;
        slot->velocity += gravityAcceleration(world, slot->position) * dt;
        slot->position += slot->velocity * dt;
        slot->lifeSeconds -= dt;

        const bool hitPlanet = std::ranges::any_of(world.planets, [&](const Planet& planet) {
            return slot->position.distanceTo(planet.position) <= planet.radius + slot->collisionRadius;
        });
        if (hitPlanet) {
            slot->exists = false;
            slot->detonated = true;
            continue;
        }

        bool impact = false;
        for (const auto& targetSlot : world.ships) {
            if (!targetSlot || !targetSlot->exists || targetSlot->combat.destroyed ||
                targetSlot->owner == slot->owner ||
                (slot->team < kTeamCount && targetSlot->team < kTeamCount &&
                 world.teams[slot->team].allied[targetSlot->team] && !world.friendlyFire)) continue;
            if (slot->position.distanceTo(targetSlot->position) <=
                slot->collisionRadius + targetSlot->collisionRadius) {
                impact = true;
                break;
            }
        }

        if (!impact && slot->lifeSeconds > 0.0) continue;
        applyTorpedoBlast(world, *slot, impact ? 250.0 : 100.0);
        slot->exists = false;
        slot->detonated = true;
    }
}

void updateCombat(World&) {
    // Weapon firing is command-driven; autonomous systems can be added here.
}

} // namespace spasim::detail
