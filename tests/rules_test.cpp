#include "engine/engine.hpp"
#include "engine/world.hpp"
#include "rules/game_rules.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{

void expect(bool condition, const char *message)
{
  if (!condition)
  {
    throw std::runtime_error(message);
  }
}

void expectNear(double actual, double expected, double tolerance, const char *message)
{
  expect(std::abs(actual - expected) <= tolerance, message);
}

} // namespace

int main()
{
  try
  {
    spasim::GameRules rules;
    auto world = spasim::makeInitialWorld();
    rules.applyScenario(world);

    expect(world.teams.size() == 5, "Scenario team count changed.");
    expect(world.planets.size() == 5, "Scenario planet count changed.");
    expect(world.weapons.size() == 4, "Scenario weapon count changed.");
    expect(world.ships.size() == 80, "Scenario ship capacity changed.");
    expect(world.ships[0] && world.ships[0]->name == "Practice target Aggstroms", "Practice ship scenario data changed.");

    expectNear(rules.orbitRadius(world.planets[0], 1), 500.0, 1e-12, "Outer orbit radius changed.");
    expectNear(rules.orbitRadius(world.planets[0], 5), 120.0, 1e-12, "Inner orbit radius changed.");
    expectNear(rules.speedLimit(3, 1000.0, 500.0), std::sqrt(2.0) * 1.5, 1e-12, "Throttle speed limit changed.");

    expectNear(rules.transferRadialStep(100.0, 1000.0, 0.1), 2.5, 1e-12, "Orbit transfer speed cap changed.");
    expectNear(rules.transferRadialStep(100.0, 99.995, 0.1), -0.005, 1e-12, "Orbit transfer close-range behavior changed.");
    expect(rules.approachThrottle(100.0, 0.0, 0.0, 20.0, 0.1) == spasim::AccelerationMode::Accelerate,
      "Approach controller should accelerate below its target speed.");
    expect(rules.approachThrottle(100.0, 20.0, 20.0, 20.0, 0.1) == spasim::AccelerationMode::Decelerate,
      "Approach controller should brake above its safe speed.");
    expect(rules.approachThrottle(100.0, 10.0, 14.0, 20.0, 0.1) == spasim::AccelerationMode::Cruise,
      "Approach controller should coast near its target speed.");

    const auto directAim = rules.weaponAimDirection({0.0, 3.0, 4.0}, {}, 0.0, {});
    expectNear(directAim.x, 0.0, 1e-12, "Direct weapon aim X changed.");
    expectNear(directAim.y, 0.6, 1e-12, "Direct weapon aim Y changed.");
    expectNear(directAim.z, 0.8, 1e-12, "Direct weapon aim Z changed.");

    spasim::WeaponAimSettings firingLimits;
    firingLimits.maximumHorizontalFiringAngleDegrees = 90.0;
    firingLimits.maximumVerticalFiringAngleDegrees = 0.0;
    expect(rules.isWithinFiringArc({-90.0, 0.0}, {1.0, 0.0, 0.0}, firingLimits),
      "Horizontal firing limit should permit its boundary.");
    expect(!rules.isWithinFiringArc({0.0, 0.0}, {0.0, 0.0, 1.0}, firingLimits),
      "Vertical firing limit should reject an out-of-range target.");

    const auto interceptAim = rules.weaponAimDirection({100.0, 0.0, 0.0}, {0.0, 10.0, 0.0}, 20.0, {});
    expect(interceptAim.y > 0.4 && interceptAim.x > 0.8, "Weapon intercept aim changed.");

    auto shortLead = world.weapons[0].aim;
    shortLead.maximumLeadSeconds = 0.5;
    const auto shortLeadAim = rules.weaponAimDirection({100.0, 0.0, 0.0}, {0.0, 10.0, 0.0}, 20.0, shortLead);
    expect(shortLeadAim.y < interceptAim.y, "Weapon-specific lead setting was ignored.");

    spasim::Simulation projectileSimulation{spasim::makeInitialWorld()};
    auto &projectileWorld = projectileSimulation.world();
    const auto projectilePlayerId = spasim::joinTeam(projectileWorld, 0, "Projectile test");
    expect(projectilePlayerId.has_value(), "Could not create projectile test player.");
    auto *projectilePlayer = projectileWorld.findPlayer(*projectilePlayerId);
    auto *projectileShooter = projectilePlayer ? projectileWorld.findShip(projectilePlayer->shipId) : nullptr;
    spasim::Ship *projectileTarget{};
    for (auto &slot : projectileWorld.ships)
    {
      if (slot && slot->name == "Practice target Fouriers")
      {
        projectileTarget = &*slot;
        break;
      }
    }
    expect(projectileShooter && projectileTarget, "Projectile test entities are missing.");
    projectileShooter->position = {};
    projectileShooter->velocity = {};
    projectileShooter->orientation = {};
    projectileShooter->resources.energy = 10000.0;
    projectileTarget->position = {100.0, 0.0, 0.0};
    projectileTarget->velocity = {};
    projectileWorld.weapons[0].projectileSpeed = 10.0;
    projectileWorld.weapons[0].aim.maximumHorizontalFiringAngleDegrees = 180.0;
    projectileWorld.weapons[0].aim.maximumVerticalFiringAngleDegrees = 180.0;
    expect(
      projectileSimulation.execute(*projectilePlayerId, spasim::SelectTarget{{spasim::Target::Kind::Entity, projectileTarget->id}})
          .status
        == spasim::CommandStatus::Applied,
      "Could not select the projectile test target.");
    const double initialShields = projectileTarget->combat.shields;
    expect(projectileSimulation.execute(*projectilePlayerId, spasim::FirePhasor{1.0}).status == spasim::CommandStatus::Applied,
      "Weapon projectile did not launch.");
    expectNear(projectileTarget->combat.shields, initialShields, 1e-12, "Weapon damage was applied before projectile impact.");
    expect(projectileWorld.weaponProjectiles.size() == 1, "Firing did not create an in-flight weapon projectile.");

    for (int step = 0; step < 4; ++step)
    {
      projectileSimulation.step();
    }

    expectNear(projectileTarget->combat.shields, initialShields, 1e-12, "Weapon damage was applied before projectile arrival.");

    for (int step = 0; step < 120 && !projectileWorld.weaponProjectiles.empty(); ++step)
    {
      projectileSimulation.step();
    }

    expect(projectileWorld.weaponProjectiles.empty(), "Weapon projectile did not resolve its impact.");
    expect(projectileTarget->combat.shields < initialShields, "Weapon projectile did not damage its target on impact.");

    projectileTarget->combat.shields = 0.0;
    projectileTarget->combat.hull = 100000.0;
    projectileTarget->combat.hullDamage = 0.0;
    for (std::size_t weaponIndex = 0; weaponIndex < projectileWorld.weapons.size(); ++weaponIndex)
    {
      auto &weapon = projectileWorld.weapons[weaponIndex];
      weapon.aim.maximumHorizontalFiringAngleDegrees = 180.0;
      weapon.aim.maximumVerticalFiringAngleDegrees = 180.0;
      expect(
        projectileSimulation.execute(*projectilePlayerId, spasim::SetWeaponProfile{static_cast<spasim::WeaponId>(weaponIndex)}).status
          == spasim::CommandStatus::Applied,
        "Could not select a weapon for damage verification.");

      const double previousDamage = projectileTarget->combat.hullDamage;
      expect(projectileSimulation.execute(*projectilePlayerId, spasim::FirePhasor{1.0}).status == spasim::CommandStatus::Applied,
        "A weapon profile could not fire at an enemy ship.");
      const auto projectileId = projectileWorld.weaponProjectiles.back().id;
      for (int step = 0; step < 500; ++step)
      {
        const auto projectile = std::find_if(projectileWorld.weaponProjectiles.begin(), projectileWorld.weaponProjectiles.end(),
          [projectileId](const spasim::WeaponProjectile &entry) { return entry.id == projectileId; });
        if (projectile == projectileWorld.weaponProjectiles.end())
        {
          break;
        }
        projectileSimulation.step();
      }
      expect(projectileTarget->combat.hullDamage > previousDamage, "A weapon profile did not damage an enemy ship.");
      projectileTarget->combat.hullDamage = 0.0;
    }

    spasim::Simulation simulation{spasim::makeInitialWorld()};
    auto &combatWorld = simulation.world();
    const auto playerId = spasim::joinTeam(combatWorld, 0, "Test");
    expect(playerId.has_value(), "Could not create combat test player.");
    auto *player = combatWorld.findPlayer(*playerId);
    auto *shooter = player ? combatWorld.findShip(player->shipId) : nullptr;
    auto *playerBase = player ? combatWorld.findBase(player->baseId) : nullptr;
    spasim::Ship *target{};
    for (auto &slot : combatWorld.ships)
    {
      if (slot && slot->name == "Practice target Fouriers")
      {
        target = &*slot;
        break;
      }
    }
    expect(player && shooter && playerBase && target, "Combat test entities are missing.");
    const auto &homePlanet = combatWorld.planets[combatWorld.teams[player->team].homePlanet];
    expect(playerBase->homePlanet == homePlanet.id, "Player base was not assigned to its team's home planet.");
    expectNear(playerBase->position.distanceTo(homePlanet.position), combatWorld.teams[player->team].baseOffset.length(), 1e-12,
      "Player base did not use its scenario-configured home-planet offset.");
    expect(playerBase->position.distanceTo(homePlanet.position) > homePlanet.radius + playerBase->collisionRadius,
      "Player base overlaps its home planet.");
    shooter->position = {};
    shooter->orientation = {-90.0, 0.0};
    shooter->resources.energy = 10000.0;
    target->position = {100.0, 0.0, 0.0};
    target->velocity = {};
    expect(simulation.execute(*playerId, spasim::SelectTarget{{spasim::Target::Kind::Entity, target->id}}).status
             == spasim::CommandStatus::Applied,
      "Could not select the combat test target.");

    combatWorld.weapons[0].aim.maximumHorizontalFiringAngleDegrees = 90.0;
    combatWorld.weapons[0].aim.maximumVerticalFiringAngleDegrees = 0.0;
    combatWorld.weapons[1].aim.maximumHorizontalFiringAngleDegrees = 0.0;
    combatWorld.weapons[1].aim.maximumVerticalFiringAngleDegrees = 0.0;
    expect(simulation.execute(*playerId, spasim::FirePhasor{1.0}).status == spasim::CommandStatus::Applied,
      "Selected weapon did not use its own aim settings.");
    expect(simulation.execute(*playerId, spasim::SetWeaponProfile{1}).status == spasim::CommandStatus::Applied,
      "Could not select the second weapon profile.");
    expect(simulation.execute(*playerId, spasim::FirePhasor{1.0}).status == spasim::CommandStatus::Invalid,
      "A different weapon incorrectly reused the first weapon's aim settings.");

    shooter->orientation = {0.0, 0.0};
    target->position = {0.0, 0.0, 100.0};
    combatWorld.weapons[2].aim.maximumHorizontalFiringAngleDegrees = 0.0;
    combatWorld.weapons[2].aim.maximumVerticalFiringAngleDegrees = 0.0;
    combatWorld.weapons[3].aim.maximumHorizontalFiringAngleDegrees = 0.0;
    combatWorld.weapons[3].aim.maximumVerticalFiringAngleDegrees = 90.0;
    expect(simulation.execute(*playerId, spasim::SetWeaponProfile{2}).status == spasim::CommandStatus::Applied,
      "Could not select the third weapon profile.");
    expect(simulation.execute(*playerId, spasim::FirePhasor{1.0}).status == spasim::CommandStatus::Invalid,
      "Vertical firing limit did not reject an out-of-range target.");
    expect(simulation.execute(*playerId, spasim::SetWeaponProfile{3}).status == spasim::CommandStatus::Applied,
      "Could not select the fourth weapon profile.");
    expect(simulation.execute(*playerId, spasim::FirePhasor{1.0}).status == spasim::CommandStatus::Applied,
      "Vertical firing limit did not accept its configured boundary.");

    spasim::Simulation loadoutSimulation{spasim::makeInitialWorld()};
    auto &loadoutWorld = loadoutSimulation.world();
    const auto loadoutPlayerId = spasim::joinTeam(loadoutWorld, 0, "Loadout test");
    auto *loadoutPlayer = loadoutPlayerId ? loadoutWorld.findPlayer(*loadoutPlayerId) : nullptr;
    auto *firstShip
      = loadoutPlayer && !loadoutPlayer->selectedShips.empty() ? loadoutWorld.findShip(loadoutPlayer->selectedShips[0]) : nullptr;
    auto *secondShip
      = loadoutPlayer && loadoutPlayer->selectedShips.size() > 1 ? loadoutWorld.findShip(loadoutPlayer->selectedShips[1]) : nullptr;
    spasim::Ship *loadoutTarget{};
    for (auto &slot : loadoutWorld.ships)
    {
      if (slot && slot->name == "Practice target Fouriers")
      {
        loadoutTarget = &*slot;
        break;
      }
    }

    expect(loadoutPlayerId && firstShip && secondShip && loadoutTarget, "Weapon loadout test entities are missing.");
    firstShip->position = {};
    secondShip->position = {0.0, 5.0, 0.0};
    firstShip->orientation = {-90.0, 0.0};
    secondShip->orientation = {-90.0, 0.0};
    firstShip->resources.energy = 10000.0;
    secondShip->resources.energy = 10000.0;
    loadoutTarget->position = {100.0, 0.0, 0.0};
    loadoutTarget->velocity = {};
    loadoutWorld.weapons[0].aim.maximumHorizontalFiringAngleDegrees = 180.0;
    loadoutWorld.weapons[1].aim.maximumHorizontalFiringAngleDegrees = 180.0;
    expect(loadoutSimulation.execute(*loadoutPlayerId, spasim::SelectTarget{{spasim::Target::Kind::Entity, loadoutTarget->id}}).status
             == spasim::CommandStatus::Applied,
      "Could not assign the loadout test target.");
    expect(loadoutSimulation.execute(*loadoutPlayerId, spasim::SelectShips{{firstShip->id}}).status == spasim::CommandStatus::Applied,
      "Could not select the first loadout test ship.");
    expect(loadoutSimulation.execute(*loadoutPlayerId, spasim::SetWeaponProfile{0}).status == spasim::CommandStatus::Applied,
      "Could not assign the first ship's weapon.");
    expect(loadoutSimulation.execute(*loadoutPlayerId, spasim::SelectShips{{secondShip->id}}).status == spasim::CommandStatus::Applied,
      "Could not select the second loadout test ship.");
    expect(loadoutSimulation.execute(*loadoutPlayerId, spasim::SetWeaponProfile{1}).status == spasim::CommandStatus::Applied,
      "Could not assign the second ship's weapon.");
    expect(firstShip->weapon == 0 && secondShip->weapon == 1, "Individual weapon profiles were not retained per ship.");
    expect(loadoutSimulation.execute(*loadoutPlayerId, spasim::SelectShips{{firstShip->id, secondShip->id}}).status
             == spasim::CommandStatus::Applied,
      "Could not select both loadout test ships.");
    expect(loadoutSimulation.execute(*loadoutPlayerId, spasim::FirePhasor{1.0}).status == spasim::CommandStatus::Applied,
      "Ships with individual weapon profiles could not fire together.");
    expect(loadoutWorld.weaponProjectiles.size() == 2, "Group fire did not launch a projectile from each ship.");

    const auto firstProjectile = std::find_if(loadoutWorld.weaponProjectiles.begin(), loadoutWorld.weaponProjectiles.end(),
      [firstShip](const spasim::WeaponProjectile &projectile) { return projectile.shooter == firstShip->id; });
    const auto secondProjectile = std::find_if(loadoutWorld.weaponProjectiles.begin(), loadoutWorld.weaponProjectiles.end(),
      [secondShip](const spasim::WeaponProjectile &projectile) { return projectile.shooter == secondShip->id; });
    expect(firstProjectile != loadoutWorld.weaponProjectiles.end() && secondProjectile != loadoutWorld.weaponProjectiles.end(),
      "Group fire did not preserve each ship's weapon profile.");
    expectNear(firstProjectile->speed, loadoutWorld.weapons[0].projectileSpeed, 1e-12,
      "First ship fired with another ship's weapon speed.");
    expectNear(firstProjectile->damage, loadoutWorld.weapons[0].damage, 1e-12, "First ship fired with another ship's weapon damage.");
    expectNear(secondProjectile->speed, loadoutWorld.weapons[1].projectileSpeed, 1e-12,
      "Second ship fired with another ship's weapon speed.");
    expectNear(secondProjectile->damage, loadoutWorld.weapons[1].damage, 1e-12,
      "Second ship fired with another ship's weapon damage.");

    const spasim::EntityId slowProjectileId = firstProjectile->id;
    const spasim::Vec3 slowProjectileStart = firstProjectile->position;
    loadoutTarget->combat.shields = 0.0;
    loadoutTarget->combat.hull = 25.0;
    loadoutTarget->combat.hullDamage = 0.0;
    for (int step = 0; step < 20 && !loadoutTarget->combat.destroyed; ++step)
    {
      loadoutSimulation.step();
    }
    expect(loadoutTarget->combat.destroyed, "Fast weapon projectile did not destroy the loadout test target.");

    loadoutSimulation.step();
    const auto remainingSlowProjectile = std::find_if(loadoutWorld.weaponProjectiles.begin(), loadoutWorld.weaponProjectiles.end(),
      [slowProjectileId](const spasim::WeaponProjectile &projectile) { return projectile.id == slowProjectileId; });
    expect(remainingSlowProjectile != loadoutWorld.weaponProjectiles.end(),
      "A projectile from a different weapon was removed when its target was destroyed.");
    expect((remainingSlowProjectile->position - slowProjectileStart).length() > 0.0,
      "The in-flight projectile stopped moving after its target was destroyed.");
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }

  return 0;
}
