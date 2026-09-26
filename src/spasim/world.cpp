#include "spasim/world.hpp"

#include <cmath>
#include <algorithm>
#include <numbers>
#include <ranges>
#include <utility>

namespace spasim
{

double Vec3::length() const noexcept
{
    return std::sqrt(x * x + y * y + z * z);
}

double Vec3::distanceTo(const Vec3& other) const noexcept
{
    return (*this - other).length();
}

Vec3& Vec3::operator+=(const Vec3& other) noexcept
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vec3& Vec3::operator-=(const Vec3& other) noexcept
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vec3 operator+(Vec3 lhs, const Vec3& rhs) noexcept
{
    return lhs += rhs;
}
Vec3 operator-(Vec3 lhs, const Vec3& rhs) noexcept
{
    return lhs -= rhs;
}
Vec3 operator*(Vec3 value, double scalar) noexcept
{
    value.x *= scalar;
    value.y *= scalar;
    value.z *= scalar;
    return value;
}

const Player* World::findPlayer(PlayerId id) const noexcept
{
    if (id >= playerSlots.size() || !playerSlots[id])
        return nullptr;
    return &*playerSlots[id];
}

Player* World::findPlayer(PlayerId id) noexcept
{
    if (id >= playerSlots.size() || !playerSlots[id])
        return nullptr;
    return &*playerSlots[id];
}

const Ship* World::findShip(EntityId id) const noexcept
{
    const auto found =
        std::ranges::find_if(ships, [id](const auto& slot) { return slot && slot->id == id; });
    return found == ships.end() ? nullptr : &**found;
}

Ship* World::findShip(EntityId id) noexcept
{
    const auto found =
        std::ranges::find_if(ships, [id](const auto& slot) { return slot && slot->id == id; });
    return found == ships.end() ? nullptr : &**found;
}

Base* World::findBase(EntityId id) noexcept
{
    const auto found =
        std::ranges::find_if(bases, [id](const auto& slot) { return slot && slot->id == id; });
    return found == bases.end() ? nullptr : &**found;
}

const Base* World::findBase(EntityId id) const noexcept
{
    const auto found =
        std::ranges::find_if(bases, [id](const auto& slot) { return slot && slot->id == id; });
    return found == bases.end() ? nullptr : &**found;
}

Torpedo* World::findTorpedo(EntityId id) noexcept
{
    const auto found =
        std::ranges::find_if(torpedoes, [id](const auto& slot) { return slot && slot->id == id; });
    return found == torpedoes.end() ? nullptr : &**found;
}

const Torpedo* World::findTorpedo(EntityId id) const noexcept
{
    const auto found =
        std::ranges::find_if(torpedoes, [id](const auto& slot) { return slot && slot->id == id; });
    return found == torpedoes.end() ? nullptr : &**found;
}

std::size_t World::activePlayerCount(TeamId team) const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        playerSlots.begin(),
        playerSlots.end(),
        [team](const auto& slot) { return slot && slot->joined && slot->team == team; }));
}

const char* toString(ObjectKind value) noexcept
{
    switch (value)
    {
    case ObjectKind::Ship:
        return "ship";
    case ObjectKind::Base:
        return "base";
    case ObjectKind::Torpedo:
        return "torpedo";
    }
    return "unknown";
}

const char* toString(WeaponKind value) noexcept
{
    switch (value)
    {
    case WeaponKind::Aggstroms:
        return "Aggstroms";
    case WeaponKind::Fouriers:
        return "Fouriers";
    case WeaponKind::Diffractions:
        return "Diffractions";
    case WeaponKind::Lasers:
        return "Lasers";
    }
    return "unknown";
}

const char* toString(AccelerationMode value) noexcept
{
    switch (value)
    {
    case AccelerationMode::Decelerate:
        return "decelerate";
    case AccelerationMode::Cruise:
        return "cruise";
    case AccelerationMode::Accelerate:
        return "accelerate";
    }
    return "unknown";
}

const char* toString(ShipClass value) noexcept
{
    switch (value)
    {
    case ShipClass::Small:
        return "small";
    case ShipClass::Medium:
        return "medium";
    case ShipClass::Large:
        return "large";
    }
    return "unknown";
}

World makeInitialWorld(std::uint64_t seed)
{
    World world;
    world.random.seed(seed);

    constexpr std::array<const char*, kTeamCount> teamNames{
        "Aggstroms", "Fouriers", "Diffractions", "Lasers"};
    for (std::size_t i = 0; i < world.teams.size(); ++i)
    {
        world.teams[i].id = static_cast<TeamId>(i);
        world.teams[i].name = teamNames[i];
        world.teams[i].allied[i] = true;
    }

    constexpr std::array<Vec3, kPlanetCount> planetPositions{
        {{-1000.0, 0.0, 0.0}, {0.0, -1000.0, 0.0}, {0.0, 0.0, -1000.0}, {1000.0, 1000.0, 1000.0}}};
    for (std::size_t i = 0; i < world.planets.size(); ++i)
    {
        auto& planet = world.planets[i];
        planet.id = static_cast<PlanetId>(i);
        planet.name = "Planet " + std::to_string(i + 1);
        planet.position = planetPositions[i];
        planet.radius = 30.0;
        planet.gravitationalParameter = 2000.0;
        planet.parkingOrbitAltitude = 90.0;
        planet.sphereOfInfluenceRadius = 500.0;
        // The printed listing gives random ranges/formulas, but the runtime's
        // operator semantics and initialized constants are not fully recoverable.
        // These neutral starting values are placeholders pending source-data recovery.
        planet.population = 100.0;
        planet.antiEntropy = 100.0;
        planet.rawMaterial = 100.0;
        planet.standardOfLiving = 0.3;
        planet.skill = 0.3;
    }
    world.weapons = {{{"Aggstroms", 10.0, 12.0, 2000.0, 0.0, false, 0.0},
                      {"Fouriers", 20.0, 20.0, 2500.0, 0.0, false, 0.0},
                      {"Diffractions", 30.0, 28.0, 3000.0, 0.0, false, 0.0},
                      {"Lasers", 15.0, 16.0, 3500.0, 0.0, false, 0.0}}};

    // Four inert practice ships provide immediate, visible targets for testing
    // selection and weapons. They sit in a compact central range near every spawn.
    constexpr std::array<Vec3, kTeamCount> practicePositions{
        {{100.0, 0.0, 0.0}, {0.0, 100.0, 0.0}, {-100.0, 0.0, 0.0}, {0.0, -100.0, 0.0}}};
    for (std::size_t i = 0; i < kPracticeShipCount; ++i)
    {
        Ship target;
        target.id = world.nextEntityId++;
        target.owner = kNoPlayer;
        target.team = static_cast<TeamId>(i);
        target.name = "Practice target " + world.teams[i].name;
        target.position = practicePositions[i];
        target.resources.energy = 10000.0;
        target.combat = {.hull = 100.0, .hullDamage = 0.0, .shields = 100.0, .skill = 0.0};
        target.acceleration = AccelerationMode::Cruise;
        target.playerControlled = false;
        world.ships[i] = std::move(target);
    }
    return world;
}

std::optional<PlayerId> joinTeam(World& world, TeamId team, std::string callSign)
{
    if (team >= kTeamCount || callSign.empty() || callSign.size() > 20 ||
        world.activePlayerCount(team) >= kPlayersPerTeam)
    {
        return std::nullopt;
    }

    const auto playerSlot =
        std::ranges::find_if(world.playerSlots, [](const auto& slot) { return !slot; });
    const auto shipSlot = std::ranges::find_if(world.ships, [](const auto& slot) { return !slot; });
    const auto baseSlot = std::ranges::find_if(world.bases, [](const auto& slot) { return !slot; });
    const auto torpedoSlot =
        std::ranges::find_if(world.torpedoes, [](const auto& slot) { return !slot; });
    if (playerSlot == world.playerSlots.end() || shipSlot == world.ships.end() ||
        baseSlot == world.bases.end() || torpedoSlot == world.torpedoes.end())
        return std::nullopt;
    const auto playerIndex = static_cast<std::size_t>(playerSlot - world.playerSlots.begin());
    const auto id = static_cast<PlayerId>(playerIndex);
    {
        const auto planetId = static_cast<PlanetId>(team);
        std::uniform_real_distribution<double> vertical(-1.0, 1.0);
        std::uniform_real_distribution<double> azimuth(0.0, 2.0 * std::numbers::pi);
        const double z = vertical(world.random);
        const double angle = azimuth(world.random);
        const double planar = std::sqrt(1.0 - z * z);
        const Vec3 spawn =
            world.planets[planetId].position +
            Vec3{70.0 * planar * std::cos(angle), 70.0 * planar * std::sin(angle), 70.0 * z};

        Player player;
        player.id = id;
        player.team = team;
        player.callSign = std::move(callSign);
        player.joined = true;
        player.shipId = world.nextEntityId++;
        player.baseId = world.nextEntityId++;
        player.torpedoId = world.nextEntityId++;
        player.selectedObject = player.shipId;

        Ship ship;
        ship.id = player.shipId;
        ship.owner = id;
        ship.team = team;
        ship.name = player.callSign;
        ship.playerControlled = true;
        ship.position = spawn;
        ship.resources.energy = 10000.0;
        ship.combat = {.hull = 100.0, .hullDamage = 0.0, .shields = 100.0, .skill = 1.0};

        Base base;
        base.id = player.baseId;
        base.owner = id;
        base.team = team;
        base.homePlanet = planetId;
        base.position = world.planets[planetId].position;
        base.resources = {.energy = 10000.0, .rawMaterial = 100.0};
        base.combat = {.hull = 100.0, .hullDamage = 0.0, .shields = 100.0, .skill = 1.0};

        Torpedo torpedo;
        torpedo.id = player.torpedoId;
        torpedo.owner = id;
        torpedo.team = team;
        torpedo.position = spawn;

        world.playerSlots[playerIndex] = std::move(player);
        *shipSlot = ship;
        *baseSlot = base;
        *torpedoSlot = torpedo;
        return id;
    }
    return std::nullopt;
}

void removePlayer(World& world, PlayerId playerId)
{
    if (playerId >= world.playerSlots.size() || !world.playerSlots[playerId])
        return;
    const auto player = *world.playerSlots[playerId];
    world.playerSlots[playerId].reset();
    for (auto& slot : world.ships)
        if (slot && slot->id == player.shipId)
            slot.reset();
    for (auto& slot : world.bases)
        if (slot && slot->id == player.baseId)
            slot.reset();
    for (auto& slot : world.torpedoes)
        if (slot && slot->id == player.torpedoId)
            slot.reset();
}

} // namespace spasim
