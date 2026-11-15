#include "engine/engine.hpp"
#include "engine/world.hpp"
#include "rules/game_rules.hpp"

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
    expect(world.ships[0] && world.ships[0]->name == "Practice target Aggstroms",
      "Practice ship scenario data changed.");

    expectNear(rules.orbitRadius(world.planets[0], 1), 500.0, 1e-12, "Outer orbit radius changed.");
    expectNear(rules.orbitRadius(world.planets[0], 5), 120.0, 1e-12, "Inner orbit radius changed.");
    expectNear(rules.speedLimit(3, 1000.0, 500.0), std::sqrt(2.0) * 1.5, 1e-12,
      "Throttle speed limit changed.");

    expectNear(rules.transferRadialStep(100.0, 1000.0, 0.1), 2.5, 1e-12, "Orbit transfer speed cap changed.");
    expectNear(rules.transferRadialStep(100.0, 99.995, 0.1), -0.005, 1e-12,
      "Orbit transfer close-range behavior changed.");
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

    spasim::Simulation simulation{spasim::makeInitialWorld()};
    auto &combatWorld = simulation.world();
    const auto playerId = spasim::joinTeam(combatWorld, 0, "Test");
    expect(playerId.has_value(), "Could not create combat test player.");
    auto *player = combatWorld.findPlayer(*playerId);
    auto *shooter = player ? combatWorld.findShip(player->shipId) : nullptr;
    spasim::Ship *target{};
    for (auto &slot : combatWorld.ships)
    {
      if (slot && slot->name == "Practice target Fouriers")
      {
        target = &*slot;
        break;
      }
    }
    expect(player && shooter && target, "Combat test entities are missing.");
    shooter->position = {};
    shooter->orientation = {-90.0, 0.0};
    shooter->resources.energy = 10000.0;
    target->position = {100.0, 0.0, 0.0};
    target->velocity = {};
    expect(simulation.execute(*playerId,
             spasim::SelectTarget{{spasim::Target::Kind::Entity, target->id}}).status == spasim::CommandStatus::Applied,
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
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }

  return 0;
}
