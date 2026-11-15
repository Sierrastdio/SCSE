#include "rules/game_rules.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace spasim
{
namespace
{

Ship makeShipDefaults(double turnRate, double skill)
{
  Ship ship;
  ship.shipClass = ShipClass::Small;
  ship.turnRateDegreesPerSecond = turnRate;
  ship.throttleLevel = 3;
  ship.collisionRadius = 20.0;
  ship.acceleration = AccelerationMode::Cruise;
  ship.resources.energy = 10000.0;
  ship.combat = {100.0, 0.0, 100.0, skill, false};
  ship.exists = true;
  return ship;
}

Team makeTeam(TeamId id, const char *name)
{
  Team team;
  team.id = id;
  team.homePlanet = static_cast<PlanetId>(id);
  team.shipClass = ShipClass::Small;
  team.name = name;
  team.spawnRadius = 70.0;
  team.spawnVerticalMinimum = -1.0;
  team.spawnVerticalMaximum = 1.0;
  team.randomSpawn = true;
  team.allies.push_back(id);
  return team;
}

Planet makePlanet(PlanetId id, const char *name, Vec3 position, double radius, double gravity)
{
  Planet planet;
  planet.id = id;
  planet.name = name;
  planet.position = position;
  planet.radius = radius;
  planet.gravitationalParameter = gravity;
  planet.parkingOrbitAltitude = 90.0;
  planet.sphereOfInfluenceRadius = 500.0;
  planet.population = 100.0;
  planet.antiEntropy = 100.0;
  planet.rawMaterial = 100.0;
  planet.standardOfLiving = 0.3;
  planet.skill = 0.3;
  return planet;
}

} // namespace

GameRules::GameRules()
{
  firstEntityId = 1;
  firstPlayerId = 0;
  defaultWeapon = 0;
  playersPerTeam = 7;
  shipsPerPlayer = 2;

  randomSeed = 0x53504153494DULL;
  fixedStepSeconds = 0.1;
  sessionDurationSeconds = 900.0;
  friendlyFire = false;
  paused = false;

  playerDefaults.goalOrbitLevel = 1;
  playerDefaults.phasorPower = 1.0;
  playerDefaults.phasorDiameter = 1.0;
  playerDefaults.weapon = defaultWeapon;

  playerShipDefaults = makeShipDefaults(30.0, 1.0);
  practiceShipDefaults = makeShipDefaults(0.0, 0.0);

  baseDefaults.collisionRadius = 50.0;
  baseDefaults.resources = {10000.0, 100.0};
  baseDefaults.combat = {100.0, 0.0, 100.0, 1.0, false};
  baseDefaults.exists = true;

  torpedoLaunchSpeed = 100.0;
  torpedoLifeSeconds = 60.0;
  torpedoCollisionRadius = 2.0;

  teams = {
    makeTeam(0, "team1"),
    makeTeam(1, "team2"),
    makeTeam(2, "team3"),
    makeTeam(3, "team4"),
    makeTeam(4, "team5"),
  };

  planets = {
    makePlanet(0, "Planet 1", {-1000.0, 0.0, 0.0}, 30.0, 2000.0),
    makePlanet(1, "Planet 2", {1000.0, 0.0, 0.0}, 30.0, 2000.0),
    makePlanet(2, "Planet 3", {0.0, 1000.0, 0.0}, 30.0, 2000.0),
    makePlanet(3, "Planet 4", {0.0, 0.0, 1000.0}, 30.0, 2000.0),
    makePlanet(4, "Planet 5", {0.0, 1000.0, 500.0}, 50.0, 20000.0),
  };

  weapons = {
    {
      .name = "MG",
      .energyCost = 1.0,
      .damage = 9.0,
      .range = 800.0,
      .projectileSpeed = 10.0,
      .explosive = false,
      .splashRadius = 0.0,
      .aim = {.maximumLeadSeconds = 60.0, .maximumHorizontalFiringAngleDegrees = 170.0, .maximumVerticalFiringAngleDegrees = 180.0},
    },
    {
      .name = "C1",
      .energyCost = 20.0,
      .damage = 50.0,
      .range = 2500.0,
      .projectileSpeed = 100.0,
      .explosive = true,
      .splashRadius = 10.0,
      .aim = {.maximumLeadSeconds = 60.0, .maximumHorizontalFiringAngleDegrees = 160.0, .maximumVerticalFiringAngleDegrees = 150.0},
    },
    {
      .name = "C2",
      .energyCost = 50.0,
      .damage = 125.0,
      .range = 3000.0,
      .projectileSpeed = 500.0,
      .explosive = false,
      .splashRadius = 15.0,
      .aim = {.maximumLeadSeconds = 60.0, .maximumHorizontalFiringAngleDegrees = 100.0, .maximumVerticalFiringAngleDegrees = 100.0},
    },
    {
      .name = "B1",
      .energyCost = 5.0,
      .damage = 10.0,
      .range = 400.0,
      .projectileSpeed = 5.0,
      .explosive = false,
      .splashRadius = 0.0,
      .aim = {.maximumLeadSeconds = 60.0, .maximumHorizontalFiringAngleDegrees = 180.0, .maximumVerticalFiringAngleDegrees = 150.0},
    },
  };

  practiceShips = {
    {0, "Practice target Aggstroms", {100.0, 0.0, 0.0}},
    {1, "Practice target Fouriers", {0.0, 100.0, 0.0}},
    {2, "Practice target Diffractions", {-100.0, 0.0, 0.0}},
    {3, "Practice target Lasers", {0.0, -100.0, 0.0}},
    {4, "Practice target Lasers", {0.0, -100.0, 100.0}},
    {0, "Practice target Aggstroms 2", {150.0, 0.0, 0.0}},
    {1, "Practice target Fouriers 2", {0.0, 150.0, 0.0}},
    {2, "Practice target Diffractions 2", {-150.0, 0.0, 0.0}},
    {3, "Practice target Lasers 2", {0.0, -150.0, 0.0}},
    {4, "Practice target Lasers 2", {0.0, -150.0, 100.0}},
  };

  sphereOfInfluenceRadius = 500.0;
  transferMaximumRadialSpeed = 25.0;
  transferRadialAcceleration = 12.0;
  levelCount = 5;
  maximumLevelByClass = {5, 4, 3};
  radiusFractions = {1.0, 0.75, 0.5, 0.25, 0.0};

  referenceGravitationalParameter = 1000.0;
  thrustAcceleration = 1.5;
  brakingAcceleration = 1.0;
  throttleSpeedMultipliers = {0.5, 1.0, 1.5, 2.5};

  if (defaultWeapon >= weapons.size() || teams.empty() || planets.empty() || weapons.empty())
  {
    throw std::logic_error("The built-in scenario has invalid IDs or empty required tables.");
  }
  for (std::size_t i = 0; i < teams.size(); ++i)
  {
    const auto &team = teams[i];
    if (team.id != i || team.homePlanet >= planets.size() || team.startingOrbitLevel > levelCount)
    {
      throw std::logic_error("The built-in scenario has invalid team settings.");
    }
    for (const auto ally : team.allies)
    {
      if (ally >= teams.size())
      {
        throw std::logic_error("The built-in scenario has an out-of-range allied team ID.");
      }
    }
  }
  for (std::size_t i = 0; i < planets.size(); ++i)
  {
    if (planets[i].id != i || planets[i].radius <= 0.0 || planets[i].gravitationalParameter <= 0.0
        || planets[i].sphereOfInfluenceRadius <= 0.0)
    {
      throw std::logic_error("The built-in scenario has invalid planet settings.");
    }
  }
  for (const auto &practiceShip : practiceShips)
  {
    if (practiceShip.team >= teams.size())
    {
      throw std::logic_error("The built-in scenario has an out-of-range practice ship team ID.");
    }
  }
  for (const auto &weapon : weapons)
  {
    const auto &aim = weapon.aim;
    if (!std::isfinite(aim.maximumLeadSeconds) || aim.maximumLeadSeconds <= 0.0
        || !std::isfinite(aim.maximumHorizontalFiringAngleDegrees) || aim.maximumHorizontalFiringAngleDegrees < 0.0
        || aim.maximumHorizontalFiringAngleDegrees > 180.0 || !std::isfinite(aim.maximumVerticalFiringAngleDegrees)
        || aim.maximumVerticalFiringAngleDegrees < 0.0 || aim.maximumVerticalFiringAngleDegrees > 180.0)
    {
      throw std::logic_error("The built-in scenario has invalid weapon aim settings.");
    }
  }
}

void GameRules::applyScenario(World &world) const
{
  const auto playerCapacity = teams.size() * playersPerTeam;
  const auto shipCapacity = playerCapacity * shipsPerPlayer + practiceShips.size();
  const auto torpedoCapacity = playerCapacity * shipsPerPlayer;
  world.nextEntityId = firstEntityId;
  world.entityIdStart = firstEntityId;
  world.playerIdStart = firstPlayerId;
  world.playersPerTeam = playersPerTeam;
  world.shipsPerPlayer = shipsPerPlayer;
  world.defaultWeapon = defaultWeapon;
  world.random.seed(world.randomSeedOverride.value_or(randomSeed));
  world.randomSeedOverride.reset();
  world.fixedStepSeconds = fixedStepSeconds;
  world.sessionDurationSeconds = sessionDurationSeconds;
  world.friendlyFire = friendlyFire;
  world.paused = paused;
  world.playerDefaults = playerDefaults;
  world.playerShipDefaults = playerShipDefaults;
  world.practiceShipDefaults = practiceShipDefaults;
  world.baseDefaults = baseDefaults;
  world.torpedoLaunchSpeed = torpedoLaunchSpeed;
  world.torpedoLifeSeconds = torpedoLifeSeconds;
  world.torpedoCollisionRadius = torpedoCollisionRadius;
  world.teams = teams;
  world.planets = planets;
  world.weapons = weapons;
  world.playerSlots.resize(playerCapacity);
  world.ships.resize(shipCapacity);
  world.bases.resize(playerCapacity);
  world.torpedoes.resize(torpedoCapacity);

  for (std::size_t i = 0; i < practiceShips.size(); ++i)
  {
    const auto &definition = practiceShips[i];
    Ship target = practiceShipDefaults;
    target.id = world.nextEntityId++;
    target.owner = kNoPlayer;
    target.team = definition.team;
    target.playerControlled = false;
    target.name = definition.name;
    target.position = definition.position;
    world.ships[i] = std::move(target);
  }
}

} // namespace spasim
