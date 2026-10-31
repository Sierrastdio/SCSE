#include "lua_rules/internal.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace spasim::lua_detail
{
void readScenario(lua_State* state, int rulesReference, GameRules& rules)
{
    lua_rawgeti(state, LUA_REGISTRYINDEX, rulesReference);
    lua_getfield(state, -1, "scenario");
    requireTable(state, -1, "scenario");
    const int scenario = lua_absindex(state, -1);

    lua_getfield(state, scenario, "ids");
    requireTable(state, -1, "scenario.ids");
    const double firstEntityId = readNumber(state, -1, "first_entity");
    const double firstPlayerId = readNumber(state, -1, "first_player");
    const double defaultWeapon = readNumber(state, -1, "default_weapon");
    constexpr auto maxEntityId = std::numeric_limits<EntityId>::max();
    if (std::floor(firstEntityId) != firstEntityId || firstEntityId < 1.0 ||
        firstEntityId > static_cast<double>(maxEntityId) ||
        std::floor(firstPlayerId) != firstPlayerId || firstPlayerId < 0.0 ||
        firstPlayerId >= static_cast<double>(kNoPlayer) ||
        std::floor(defaultWeapon) != defaultWeapon || defaultWeapon < 0.0 ||
        defaultWeapon > static_cast<double>(std::numeric_limits<WeaponId>::max()))
        throw std::runtime_error("Lua rules: invalid scenario ID settings.");
    rules.firstEntityId = static_cast<EntityId>(firstEntityId);
    rules.firstPlayerId = static_cast<PlayerId>(firstPlayerId);
    rules.defaultWeapon = static_cast<WeaponId>(defaultWeapon);
    lua_pop(state, 1);

    lua_getfield(state, scenario, "world");
    requireTable(state, -1, "scenario.world");
    const double seed = readNumber(state, -1, "random_seed");
    if (std::floor(seed) != seed || seed < 0.0 || seed > 9007199254740991.0)
        throw std::runtime_error("Lua rules: random_seed must be a non-negative exact integer.");
    rules.randomSeed = static_cast<std::uint64_t>(seed);
    rules.fixedStepSeconds = readNumber(state, -1, "fixed_step_seconds");
    rules.sessionDurationSeconds = readNumber(state, -1, "session_duration_seconds");
    rules.friendlyFire = readBoolean(state, -1, "friendly_fire");
    rules.paused = readBoolean(state, -1, "paused");
    if (rules.fixedStepSeconds <= 0.0 || rules.sessionDurationSeconds <= 0.0)
        throw std::runtime_error("Lua rules: world timing values must be positive.");
    lua_pop(state, 1);

    lua_getfield(state, scenario, "player_defaults");
    requireTable(state, -1, "scenario.player_defaults");
    const double goalOrbitLevel = readNumber(state, -1, "goal_orbit_level");
    if (std::floor(goalOrbitLevel) != goalOrbitLevel || goalOrbitLevel < 1.0 ||
        goalOrbitLevel > kMaxOrbitLevels)
        throw std::runtime_error("Lua rules: player goal_orbit_level is out of range.");
    rules.playerDefaults.goalOrbitLevel = static_cast<std::uint8_t>(goalOrbitLevel);
    rules.playerDefaults.phasorPower = readNumber(state, -1, "phasor_power");
    rules.playerDefaults.phasorDiameter = readNumber(state, -1, "phasor_diameter");
    rules.playerDefaults.weapon = rules.defaultWeapon;
    if (rules.playerDefaults.phasorPower <= 0.0 || rules.playerDefaults.phasorDiameter <= 0.0)
        throw std::runtime_error("Lua rules: invalid player defaults.");
    lua_pop(state, 1);

    lua_getfield(state, scenario, "player_ship_defaults");
    requireTable(state, -1, "scenario.player_ship_defaults");
    readShipDefaults(state, -1, rules.playerShipDefaults);
    lua_pop(state, 1);

    lua_getfield(state, scenario, "practice_ship_defaults");
    requireTable(state, -1, "scenario.practice_ship_defaults");
    readShipDefaults(state, -1, rules.practiceShipDefaults);
    lua_pop(state, 1);

    lua_getfield(state, scenario, "base_defaults");
    requireTable(state, -1, "scenario.base_defaults");
    rules.baseDefaults.collisionRadius = readNumber(state, -1, "collision_radius");
    rules.baseDefaults.resources.energy = readNumber(state, -1, "energy");
    rules.baseDefaults.resources.rawMaterial = readNumber(state, -1, "material");
    rules.baseDefaults.combat.hull = readNumber(state, -1, "hull");
    rules.baseDefaults.combat.hullDamage = readNumber(state, -1, "hull_damage");
    rules.baseDefaults.combat.shields = readNumber(state, -1, "shields");
    rules.baseDefaults.combat.skill = readNumber(state, -1, "skill");
    rules.baseDefaults.combat.destroyed = readBoolean(state, -1, "destroyed");
    rules.baseDefaults.exists = true;
    if (rules.baseDefaults.collisionRadius <= 0.0 || rules.baseDefaults.resources.energy < 0.0 ||
        rules.baseDefaults.resources.rawMaterial < 0.0 || rules.baseDefaults.combat.hull < 0.0 ||
        rules.baseDefaults.combat.shields < 0.0)
        throw std::runtime_error("Lua rules: invalid base defaults.");
    lua_pop(state, 1);

    lua_getfield(state, scenario, "torpedo_defaults");
    requireTable(state, -1, "scenario.torpedo_defaults");
    rules.torpedoLaunchSpeed = readNumber(state, -1, "launch_speed");
    rules.torpedoLifeSeconds = readNumber(state, -1, "life_seconds");
    rules.torpedoCollisionRadius = readNumber(state, -1, "collision_radius");
    if (rules.torpedoLaunchSpeed <= 0.0 || rules.torpedoLifeSeconds <= 0.0 ||
        rules.torpedoCollisionRadius <= 0.0)
        throw std::runtime_error("Lua rules: invalid torpedo defaults.");
    lua_pop(state, 1);

    lua_getfield(state, scenario, "players_per_team");
    const double playersPerTeam = lua_tonumber(state, -1);
    if (!lua_isnumber(state, -1) || std::floor(playersPerTeam) != playersPerTeam ||
        playersPerTeam < 1.0 || playersPerTeam > 65535.0)
        throw std::runtime_error("Lua rules: players_per_team must be an integer from 1 to 65535.");
    rules.playersPerTeam = static_cast<std::size_t>(playersPerTeam);
    lua_pop(state, 1);

    lua_getfield(state, scenario, "teams");
    requireTable(state, -1, "scenario.teams");
    const auto teamCount = lua_rawlen(state, -1);
    if (teamCount == 0 || teamCount > 65536)
        throw std::runtime_error("Lua rules: scenario.teams must contain 1-65536 teams.");
    if (teamCount * rules.playersPerTeam > 65535)
        throw std::runtime_error("Lua rules: total player capacity cannot exceed 65535.");
    rules.teams.reserve(teamCount);
    for (std::size_t i = 0; i < teamCount; ++i)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(i + 1));
        requireTable(state, -1, "scenario.teams entry");
        requireDenseId(state, -1, i);
        rules.teams.emplace_back();
        auto& team = rules.teams.back();
        team.id = static_cast<TeamId>(i);
        const double homePlanet = readNumber(state, -1, "home_planet_id");
        if (std::floor(homePlanet) != homePlanet || homePlanet < 0.0 ||
            homePlanet > std::numeric_limits<PlanetId>::max())
            throw std::runtime_error("Lua rules: team home_planet_id is out of range.");
        team.homePlanet = static_cast<PlanetId>(homePlanet);
        team.name = readString(state, -1, "name");
        team.shipClass = readShipClass(state, -1, "ship_class");
        const double startingOrbit = readNumber(state, -1, "starting_orbit_level");
        const double spawnRadius = readNumber(state, -1, "spawn_radius");
        if (std::floor(startingOrbit) != startingOrbit || startingOrbit < 0.0 ||
            startingOrbit > kMaxOrbitLevels || spawnRadius < 0.0)
            throw std::runtime_error("Lua rules: invalid team spawn or starting orbit.");
        team.startingOrbitLevel = static_cast<std::uint8_t>(startingOrbit);
        team.spawnRadius = spawnRadius;
        team.spawnOffset = readVector(state, -1, "spawn_offset");
        team.baseOffset = readVector(state, -1, "base_offset");
        team.spawnVerticalMinimum = readNumber(state, -1, "spawn_vertical_minimum");
        team.spawnVerticalMaximum = readNumber(state, -1, "spawn_vertical_maximum");
        team.randomSpawn = readBoolean(state, -1, "random_spawn");
        if (team.spawnVerticalMinimum < -1.0 || team.spawnVerticalMaximum > 1.0 ||
            team.spawnVerticalMinimum >= team.spawnVerticalMaximum)
            throw std::runtime_error("Lua rules: invalid spawn vertical range.");
        lua_getfield(state, -1, "allied");
        requireTable(state, -1, "team.allied");
        const int allies = lua_absindex(state, -1);
        const auto allyCount = lua_rawlen(state, allies);
        for (std::size_t ally = 0; ally < allyCount; ++ally)
        {
            lua_geti(state, allies, static_cast<lua_Integer>(ally + 1));
            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                break;
            }
            const bool valid = lua_isinteger(state, -1);
            const lua_Integer allyId = lua_tointeger(state, -1);
            lua_pop(state, 1);
            if (!valid || allyId < 0 || allyId >= static_cast<lua_Integer>(teamCount))
                throw std::runtime_error("Lua rules: allied team ID is out of range.");
            team.allies.push_back(static_cast<TeamId>(allyId));
        }
        lua_pop(state, 1);
        lua_pop(state, 1);
    }
    lua_pop(state, 1);

    lua_getfield(state, scenario, "planets");
    requireTable(state, -1, "scenario.planets");
    const auto planetCount = lua_rawlen(state, -1);
    if (planetCount == 0 ||
        planetCount > static_cast<std::size_t>(std::numeric_limits<PlanetId>::max()) + 1)
        throw std::runtime_error("Lua rules: scenario.planets must contain 1-256 planets.");
    rules.planets.reserve(planetCount);
    for (std::size_t i = 0; i < planetCount; ++i)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(i + 1));
        requireTable(state, -1, "scenario.planets entry");
        const int planetIndex = lua_absindex(state, -1);
        requireDenseId(state, planetIndex, i);
        rules.planets.emplace_back();
        auto& planet = rules.planets.back();
        planet.id = static_cast<PlanetId>(i);
        planet.name = readString(state, planetIndex, "name");
        planet.position = readVector(state, planetIndex, "position");
        planet.radius = readNumber(state, planetIndex, "radius");
        planet.gravitationalParameter = readNumber(state, planetIndex, "gravity");
        planet.parkingOrbitAltitude = readNumber(state, planetIndex, "parking_altitude");
        planet.sphereOfInfluenceRadius =
            readNumber(state, planetIndex, "sphere_of_influence_radius");
        planet.population = readNumber(state, planetIndex, "population");
        planet.antiEntropy = readNumber(state, planetIndex, "energy");
        planet.rawMaterial = readNumber(state, planetIndex, "material");
        planet.standardOfLiving = readNumber(state, planetIndex, "standard_of_living");
        planet.skill = readNumber(state, planetIndex, "skill");
        const double controllingTeam = readNumber(state, planetIndex, "controlling_team_id");
        planet.revoltAtSeconds = readNumber(state, planetIndex, "revolt_at_seconds");
        if (planet.radius <= 0.0 || planet.gravitationalParameter <= 0.0 ||
            planet.parkingOrbitAltitude < 0.0 || planet.sphereOfInfluenceRadius <= 0.0 ||
            planet.population < 0.0 || planet.antiEntropy < 0.0 || planet.rawMaterial < 0.0 ||
            (controllingTeam != -1.0 && (std::floor(controllingTeam) != controllingTeam ||
                                         controllingTeam < 0.0 || controllingTeam >= teamCount)) ||
            planet.revoltAtSeconds < 0.0)
            throw std::runtime_error("Lua rules: invalid planet values in scenario.");
        if (controllingTeam >= 0.0)
            planet.controllingTeam = static_cast<TeamId>(controllingTeam);
        lua_pop(state, 1);
    }
    lua_pop(state, 1);
    for (const auto& team : rules.teams)
        if (team.homePlanet >= rules.planets.size())
            throw std::runtime_error("Lua rules: team home_planet_id is out of range.");

    lua_getfield(state, scenario, "weapons");
    requireTable(state, -1, "scenario.weapons");
    const auto weaponCount = lua_rawlen(state, -1);
    if (weaponCount == 0 || weaponCount > 65536)
        throw std::runtime_error("Lua rules: scenario.weapons must contain 1-65536 weapons.");
    rules.weapons.reserve(weaponCount);
    for (std::size_t i = 0; i < weaponCount; ++i)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(i + 1));
        requireTable(state, -1, "scenario.weapons entry");
        requireDenseId(state, -1, i);
        rules.weapons.emplace_back();
        auto& weapon = rules.weapons.back();
        weapon.name = readString(state, -1, "name");
        weapon.energyCost = readNumber(state, -1, "energy_cost");
        weapon.damage = readNumber(state, -1, "damage");
        weapon.range = readNumber(state, -1, "range");
        weapon.projectileSpeed = readNumber(state, -1, "projectile_speed");
        lua_getfield(state, -1, "explosive");
        const bool valid = lua_isboolean(state, -1);
        weapon.explosive = lua_toboolean(state, -1) != 0;
        lua_pop(state, 1);
        if (!valid)
            throw std::runtime_error("Lua rules: weapon explosive must be a boolean.");
        weapon.splashRadius = readNumber(state, -1, "splash_radius");
        if (weapon.energyCost < 0.0 || weapon.damage < 0.0 || weapon.range <= 0.0 ||
            weapon.projectileSpeed < 0.0 || weapon.splashRadius < 0.0)
            throw std::runtime_error("Lua rules: invalid weapon values in scenario.");
        lua_pop(state, 1);
    }
    lua_pop(state, 1);

    if (rules.defaultWeapon >= rules.weapons.size())
        throw std::runtime_error("Lua rules: default_weapon ID is out of range.");

    lua_getfield(state, scenario, "practice_ships");
    requireTable(state, -1, "scenario.practice_ships");
    const auto practiceCount = lua_rawlen(state, -1);
    if (practiceCount + teamCount * rules.playersPerTeam > 65535)
        throw std::runtime_error(
            "Lua rules: player and practice ship capacity cannot exceed 65535.");
    rules.practiceNames.reserve(practiceCount);
    rules.practicePositions.reserve(practiceCount);
    rules.practiceTeams.reserve(practiceCount);
    for (std::size_t i = 0; i < practiceCount; ++i)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(i + 1));
        requireTable(state, -1, "scenario.practice_ships entry");
        const int targetIndex = lua_absindex(state, -1);
        const double teamId = readNumber(state, targetIndex, "team_id");
        if (std::floor(teamId) != teamId || teamId < 0.0 || teamId >= teamCount)
            throw std::runtime_error("Lua rules: practice ship team_id is out of range.");
        rules.practiceTeams.push_back(static_cast<TeamId>(teamId));
        rules.practiceNames.push_back(readString(state, targetIndex, "name"));
        rules.practicePositions.push_back(readVector(state, targetIndex, "position"));
        lua_pop(state, 1);
    }
    const auto playerCapacity = teamCount * rules.playersPerTeam;
    const auto allocatedEntityIds = practiceCount + playerCapacity * 3;
    if (firstEntityId + static_cast<double>(allocatedEntityIds) > maxEntityId ||
        firstPlayerId + static_cast<double>(playerCapacity) > kNoPlayer)
        throw std::runtime_error("Lua rules: configured team/player counts overflow IDs.");
    lua_pop(state, 3);
}
} // namespace spasim::lua_detail



