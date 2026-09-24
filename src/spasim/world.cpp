#include "spasim/world.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace spasim {

double Vec3::length() const noexcept { return std::sqrt(x * x + y * y + z * z); }

double Vec3::distanceTo(const Vec3& other) const noexcept {
    return (*this - other).length();
}

Vec3& Vec3::operator+=(const Vec3& other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vec3& Vec3::operator-=(const Vec3& other) noexcept {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vec3 operator+(Vec3 lhs, const Vec3& rhs) noexcept { return lhs += rhs; }
Vec3 operator-(Vec3 lhs, const Vec3& rhs) noexcept { return lhs -= rhs; }
Vec3 operator*(Vec3 value, double scalar) noexcept {
    value.x *= scalar;
    value.y *= scalar;
    value.z *= scalar;
    return value;
}

const Player* World::findPlayer(PlayerId id) const noexcept {
    if (id >= playerSlots.size() || !playerSlots[id]) return nullptr;
    return &*playerSlots[id];
}

Player* World::findPlayer(PlayerId id) noexcept {
    if (id >= playerSlots.size() || !playerSlots[id]) return nullptr;
    return &*playerSlots[id];
}

const Ship* World::findShip(EntityId id) const noexcept {
    const auto found = std::ranges::find(ships, id, &Ship::id);
    return found == ships.end() ? nullptr : &*found;
}

Ship* World::findShip(EntityId id) noexcept {
    const auto found = std::ranges::find(ships, id, &Ship::id);
    return found == ships.end() ? nullptr : &*found;
}

Base* World::findBase(EntityId id) noexcept {
    const auto found = std::ranges::find(bases, id, &Base::id);
    return found == bases.end() ? nullptr : &*found;
}

Torpedo* World::findTorpedo(EntityId id) noexcept {
    const auto found = std::ranges::find(torpedoes, id, &Torpedo::id);
    return found == torpedoes.end() ? nullptr : &*found;
}

std::size_t World::activePlayerCount(TeamId team) const noexcept {
    return static_cast<std::size_t>(std::ranges::count_if(playerSlots, [team](const auto& slot) {
        return slot && slot->joined && slot->team == team;
    }));
}

const char* toString(ObjectKind value) noexcept {
    switch (value) {
    case ObjectKind::Ship: return "ship";
    case ObjectKind::Base: return "base";
    case ObjectKind::Torpedo: return "torpedo";
    }
    return "unknown";
}

const char* toString(WeaponKind value) noexcept {
    switch (value) {
    case WeaponKind::Aggstroms: return "Aggstroms";
    case WeaponKind::Fouriers: return "Fouriers";
    case WeaponKind::Diffractions: return "Diffractions";
    case WeaponKind::Lasers: return "Lasers";
    }
    return "unknown";
}

const char* toString(AccelerationMode value) noexcept {
    switch (value) {
    case AccelerationMode::Decelerate: return "decelerate";
    case AccelerationMode::Cruise: return "cruise";
    case AccelerationMode::Accelerate: return "accelerate";
    }
    return "unknown";
}

World makeInitialWorld(std::uint64_t seed) {
    World world;
    world.random.seed(seed);

    constexpr std::array<const char*, kTeamCount> teamNames{
        "Aggstroms", "Fouriers", "Diffractions", "Lasers"};
    for (std::size_t i = 0; i < world.teams.size(); ++i) {
        world.teams[i].id = static_cast<TeamId>(i);
        world.teams[i].name = teamNames[i];
        world.teams[i].allied[i] = true;
    }

    constexpr std::array<Vec3, kPlanetCount> planetPositions{{
        {-1000.0, 0.0, 0.0}, {0.0, -1000.0, 0.0},
        {0.0, 0.0, -1000.0}, {1000.0, 1000.0, 1000.0}}};
    for (std::size_t i = 0; i < world.planets.size(); ++i) {
        auto& planet = world.planets[i];
        planet.id = static_cast<PlanetId>(i);
        planet.name = "Planet " + std::to_string(i + 1);
        planet.position = planetPositions[i];
        // The printed listing gives random ranges/formulas, but the runtime's
        // operator semantics and initialized constants are not fully recoverable.
        // These neutral starting values are placeholders pending source-data recovery.
        planet.population = 100.0;
        planet.antiEntropy = 100.0;
        planet.rawMaterial = 100.0;
        planet.standardOfLiving = 0.3;
        planet.skill = 0.3;
    }
    return world;
}

std::optional<PlayerId> joinTeam(World& world, TeamId team, std::string callSign) {
    if (team >= kTeamCount || callSign.empty() || callSign.size() > 20 ||
        world.activePlayerCount(team) >= kPlayersPerTeam) {
        return std::nullopt;
    }

    for (PlayerId id = 0; id < world.playerSlots.size(); ++id) {
        if (world.playerSlots[id]) continue;
        const auto planetId = static_cast<PlanetId>(team);
        std::uniform_real_distribution<double> offset(-50.0, 50.0);
        const Vec3 spawn{world.planets[planetId].position.x + offset(world.random),
                         world.planets[planetId].position.y + offset(world.random),
                         world.planets[planetId].position.z + offset(world.random)};

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

        world.playerSlots[id] = std::move(player);
        world.ships.push_back(ship);
        world.bases.push_back(base);
        world.torpedoes.push_back(torpedo);
        return id;
    }
    return std::nullopt;
}

void removePlayer(World& world, PlayerId playerId) {
    auto* player = world.findPlayer(playerId);
    if (!player) return;
    world.ships.erase(std::remove_if(world.ships.begin(), world.ships.end(), [player](const Ship& ship) {
        return ship.id == player->shipId;
    }), world.ships.end());
    world.bases.erase(std::remove_if(world.bases.begin(), world.bases.end(), [player](const Base& base) {
        return base.id == player->baseId;
    }), world.bases.end());
    world.torpedoes.erase(std::remove_if(world.torpedoes.begin(), world.torpedoes.end(), [player](const Torpedo& torpedo) {
        return torpedo.id == player->torpedoId;
    }), world.torpedoes.end());
    world.playerSlots[playerId].reset();
}

} // namespace spasim
