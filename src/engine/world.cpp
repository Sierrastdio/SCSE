#include "engine/world.hpp"

#include <cmath>
#include <algorithm>
#include <numbers>
#include <ranges>
#include <utility>

namespace spasim
{

double Vec3::lengthSquared() const noexcept { return x * x + y * y + z * z; }

double Vec3::length() const noexcept { return std::sqrt(lengthSquared()); }

double Vec3::distanceTo(const Vec3 &other) const noexcept { return (*this - other).length(); }

Vec3 &Vec3::operator+=(const Vec3 &other) noexcept
{
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

Vec3 &Vec3::operator-=(const Vec3 &other) noexcept
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

Vec3 operator+(Vec3 lhs, const Vec3 &rhs) noexcept { return lhs += rhs; }

Vec3 operator-(Vec3 lhs, const Vec3 &rhs) noexcept { return lhs -= rhs; }

Vec3 operator*(Vec3 value, double scalar) noexcept
{
  value.x *= scalar;
  value.y *= scalar;
  value.z *= scalar;
  return value;
}

const Player *World::findPlayer(PlayerId id) const noexcept
{
  const auto found = std::ranges::find_if(playerSlots, [id](const auto &slot) { return slot && slot->id == id; });
  return found == playerSlots.end() ? nullptr : &**found;
}

Player *World::findPlayer(PlayerId id) noexcept
{
  const auto found = std::ranges::find_if(playerSlots, [id](const auto &slot) { return slot && slot->id == id; });
  return found == playerSlots.end() ? nullptr : &**found;
}

const Ship *World::findShip(EntityId id) const noexcept
{
  const auto found = std::ranges::find_if(ships, [id](const auto &slot) { return slot && slot->id == id; });
  return found == ships.end() ? nullptr : &**found;
}

Ship *World::findShip(EntityId id) noexcept
{
  const auto found = std::ranges::find_if(ships, [id](const auto &slot) { return slot && slot->id == id; });
  return found == ships.end() ? nullptr : &**found;
}

Base *World::findBase(EntityId id) noexcept
{
  const auto found = std::ranges::find_if(bases, [id](const auto &slot) { return slot && slot->id == id; });
  return found == bases.end() ? nullptr : &**found;
}

const Base *World::findBase(EntityId id) const noexcept
{
  const auto found = std::ranges::find_if(bases, [id](const auto &slot) { return slot && slot->id == id; });
  return found == bases.end() ? nullptr : &**found;
}

Torpedo *World::findTorpedo(EntityId id) noexcept
{
  const auto found = std::ranges::find_if(torpedoes, [id](const auto &slot) { return slot && slot->id == id; });
  return found == torpedoes.end() ? nullptr : &**found;
}

const Torpedo *World::findTorpedo(EntityId id) const noexcept
{
  const auto found = std::ranges::find_if(torpedoes, [id](const auto &slot) { return slot && slot->id == id; });
  return found == torpedoes.end() ? nullptr : &**found;
}

std::size_t World::activePlayerCount(TeamId team) const noexcept
{
  return static_cast<std::size_t>(std::count_if(playerSlots.begin(), playerSlots.end(),
    [team](const auto &slot) { return slot && slot->joined && slot->team == team; }));
}

const char *toString(ObjectKind value) noexcept
{
  switch (value)
  {
    case ObjectKind::Ship: return "ship";
    case ObjectKind::Base: return "base";
    case ObjectKind::Torpedo: return "torpedo";
  }
  return "unknown";
}

const char *toString(AccelerationMode value) noexcept
{
  switch (value)
  {
    case AccelerationMode::Decelerate: return "decelerate";
    case AccelerationMode::Cruise: return "cruise";
    case AccelerationMode::Accelerate: return "accelerate";
  }
  return "unknown";
}

const char *toString(ShipClass value) noexcept
{
  switch (value)
  {
    case ShipClass::Small: return "small";
    case ShipClass::Medium: return "medium";
    case ShipClass::Large: return "large";
  }
  return "unknown";
}

World makeInitialWorld(std::optional<std::uint64_t> seed)
{
  World world;
  world.randomSeedOverride = seed;
  return world;
}

std::optional<PlayerId> joinTeam(World &world, TeamId team, std::string callSign)
{
  if (team >= world.teams.size() || callSign.empty() || callSign.size() > 20 ||
      world.activePlayerCount(team) >= world.playersPerTeam || world.shipsPerPlayer == 0)
  {
    return std::nullopt;
  }

  const auto playerSlot = std::ranges::find_if(world.playerSlots, [](const auto &slot) { return !slot; });
  const auto baseSlot = std::ranges::find_if(world.bases, [](const auto &slot) { return !slot; });
  std::vector<std::optional<Ship> *> shipSlots;
  std::vector<std::optional<Torpedo> *> torpedoSlots;
  for (auto &slot : world.ships)
    if (!slot && shipSlots.size() < world.shipsPerPlayer)
      shipSlots.push_back(&slot);
  for (auto &slot : world.torpedoes)
    if (!slot && torpedoSlots.size() < world.shipsPerPlayer)
      torpedoSlots.push_back(&slot);

  if (playerSlot == world.playerSlots.end() || shipSlots.size() != world.shipsPerPlayer ||
      torpedoSlots.size() != world.shipsPerPlayer || baseSlot == world.bases.end())
    return std::nullopt;

  const auto playerIndex = static_cast<std::size_t>(playerSlot - world.playerSlots.begin());

  if (world.playerIdStart > kNoPlayer - 1 - playerIndex)
    return std::nullopt;

  const auto id = static_cast<PlayerId>(world.playerIdStart + playerIndex);
  {
    const auto planetId = world.teams[team].homePlanet;
    const auto &teamSettings = world.teams[team];

    std::uniform_real_distribution<double> vertical(teamSettings.spawnVerticalMinimum, teamSettings.spawnVerticalMaximum);
    std::uniform_real_distribution<double> azimuth(0.0, 2.0 * std::numbers::pi);
    Vec3 spawnOffset = teamSettings.spawnOffset;

    if (teamSettings.randomSpawn)
    {
      const double z = vertical(world.random);
      const double angle = azimuth(world.random);
      const double planar = std::sqrt(1.0 - z * z);
      spawnOffset = {teamSettings.spawnRadius * planar * std::cos(angle), teamSettings.spawnRadius * planar * std::sin(angle),
        teamSettings.spawnRadius * z};
    }

    const double spawnRadius = spawnOffset.length();

    Vec3 spawnDirection = spawnRadius > 1e-9 ? spawnOffset * (1.0 / spawnRadius) : Vec3{1.0, 0.0, 0.0};

    const auto startOrbit = teamSettings.startingOrbitLevel;

    if (startOrbit > 0)
      spawnOffset = spawnDirection * world.planets[planetId].orbitLevelRadii[startOrbit - 1];

    const Vec3 spawn = world.planets[planetId].position + spawnOffset;

    Player player = world.playerDefaults;
    player.id = id;
    player.team = team;
    player.callSign = std::move(callSign);
    player.joined = true;
    player.weapon = world.defaultWeapon;
    player.shipId = world.nextEntityId++;
    player.baseId = world.nextEntityId++;
    std::vector<EntityId> shipIds{player.shipId};
    std::vector<EntityId> torpedoIds{world.nextEntityId++};
    for (std::size_t index = 1; index < world.shipsPerPlayer; ++index)
    {
      shipIds.push_back(world.nextEntityId++);
      torpedoIds.push_back(world.nextEntityId++);
    }
    player.selectedShips = shipIds;
    player.selectedObject = player.shipId;

    const Vec3 axis = std::abs(spawnDirection.z) < 0.9 ? Vec3{0.0, 0.0, 1.0} : Vec3{0.0, 1.0, 0.0};
    Vec3 tangent{axis.y * spawnDirection.z - axis.z * spawnDirection.y, axis.z * spawnDirection.x - axis.x * spawnDirection.z,
      axis.x * spawnDirection.y - axis.y * spawnDirection.x};
    tangent = tangent * (1.0 / tangent.length());
    const double orbitRadius = startOrbit > 0 ? world.planets[planetId].orbitLevelRadii[startOrbit - 1] : 0.0;
    const double orbitSpeed = startOrbit > 0 ? std::sqrt(world.planets[planetId].gravitationalParameter / orbitRadius) : 0.0;

    for (std::size_t index = 0; index < shipIds.size(); ++index)
    {
      Ship ship = world.playerShipDefaults;
      ship.id = shipIds[index];
      ship.owner = id;
      ship.team = team;
      ship.name = index == 0 ? player.callSign : player.callSign + " " + std::to_string(index + 1);
      ship.shipClass = teamSettings.shipClass;
      ship.torpedoId = torpedoIds[index];
      ship.playerControlled = true;

      if (startOrbit > 0)
      {
        const double angle = static_cast<double>(index) * 50.0 / orbitRadius;
        const Vec3 radial = spawnDirection * std::cos(angle) + tangent * std::sin(angle);
        ship.position = world.planets[planetId].position + radial * orbitRadius;
        ship.orbitingPlanet = planetId;
        ship.orbitLevel = startOrbit;

        Vec3 shipTangent{
          axis.y * radial.z - axis.z * radial.y, axis.z * radial.x - axis.x * radial.z, axis.x * radial.y - axis.y * radial.x};
        shipTangent = shipTangent * (1.0 / shipTangent.length());
        ship.velocity = shipTangent * orbitSpeed;
        ship.speed = ship.warp = orbitSpeed;
      }
      else
        ship.position = spawn + tangent * (50.0 * static_cast<double>(index));

      Torpedo torpedo;
      torpedo.id = ship.torpedoId;
      torpedo.owner = id;
      torpedo.team = team;
      torpedo.position = ship.position;
      torpedo.collisionRadius = world.torpedoCollisionRadius;
      *shipSlots[index] = std::move(ship);
      *torpedoSlots[index] = torpedo;
    }

    Base base = world.baseDefaults;
    base.id = player.baseId;
    base.owner = id;
    base.team = team;
    base.homePlanet = planetId;
    base.position = world.planets[planetId].position + teamSettings.baseOffset;

    world.playerSlots[playerIndex] = std::move(player);
    *baseSlot = base;
    return id;
  }
  return std::nullopt;
}

void removePlayer(World &world, PlayerId playerId)
{
  auto found = std::ranges::find_if(world.playerSlots, [playerId](const auto &slot) { return slot && slot->id == playerId; });

  if (found == world.playerSlots.end())
    return;

  const auto player = **found;
  found->reset();

  for (auto &slot : world.ships)
    if (slot && slot->owner == playerId && slot->playerControlled)
      slot.reset();

  for (auto &slot : world.bases)
    if (slot && slot->id == player.baseId)
      slot.reset();

  for (auto &slot : world.torpedoes)
    if (slot && slot->owner == playerId)
      slot.reset();
}

} // namespace spasim
