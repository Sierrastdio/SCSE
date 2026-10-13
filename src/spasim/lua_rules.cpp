#include "spasim/lua_rules.hpp"

#include <lua.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace spasim
{
namespace
{

double readNumber(lua_State* state, int tableIndex, const char* key)
{
    lua_getfield(state, tableIndex, key);
    const bool valid = lua_isnumber(state, -1);
    const double value = lua_tonumber(state, -1);
    lua_pop(state, 1);
    if (!valid || !std::isfinite(value))
        throw std::runtime_error(std::string("Lua rules: invalid numeric field '") + key + "'.");
    return value;
}

void requireTable(lua_State* state, int index, const char* description)
{
    if (!lua_istable(state, index))
        throw std::runtime_error(std::string("Lua rules: expected table for ") + description);
}

int cacheFunction(lua_State* state, int rulesReference, const char* name)
{
    lua_rawgeti(state, LUA_REGISTRYINDEX, rulesReference);
    lua_getfield(state, -1, name);
    if (!lua_isfunction(state, -1))
        throw std::runtime_error(std::string("Lua rules: function '") + name + "' is required.");
    lua_remove(state, -2);
    return luaL_ref(state, LUA_REGISTRYINDEX);
}

void openRuleLibraries(lua_State* state)
{
    // Rules use the base and math APIs; avoid loading unused standard libraries.
    luaL_requiref(state, "_G", luaopen_base, 1);
    lua_pop(state, 1);
    luaL_requiref(state, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(state, 1);
}

std::string readString(lua_State* state, int tableIndex, const char* key)
{
    lua_getfield(state, tableIndex, key);
    const bool valid = lua_type(state, -1) == LUA_TSTRING;
    const char* value = valid ? lua_tostring(state, -1) : nullptr;
    std::string result = value ? value : "";
    lua_pop(state, 1);
    if (!valid || result.empty())
        throw std::runtime_error(std::string("Lua rules: invalid string field '") + key + "'.");
    return result;
}

Vec3 readVector(lua_State* state, int tableIndex, const char* key)
{
    lua_getfield(state, tableIndex, key);
    requireTable(state, -1, key);
    const int vectorIndex = lua_absindex(state, -1);
    Vec3 result;
    double* components[] = {&result.x, &result.y, &result.z};
    for (int i = 0; i < 3; ++i)
    {
        lua_geti(state, vectorIndex, i + 1);
        const bool valid = lua_isnumber(state, -1) && std::isfinite(lua_tonumber(state, -1));
        *components[i] = lua_tonumber(state, -1);
        lua_pop(state, 1);
        if (!valid)
            throw std::runtime_error(std::string("Lua rules: invalid vector '") + key + "'.");
    }
    lua_pop(state, 1);
    return result;
}

bool readBoolean(lua_State* state, int tableIndex, const char* key)
{
    lua_getfield(state, tableIndex, key);
    const bool valid = lua_isboolean(state, -1);
    const bool value = lua_toboolean(state, -1) != 0;
    lua_pop(state, 1);
    if (!valid)
        throw std::runtime_error(std::string("Lua rules: invalid boolean field '") + key + "'.");
    return value;
}

ShipClass readShipClass(lua_State* state, int tableIndex, const char* key)
{
    const auto value = readString(state, tableIndex, key);
    if (value == "small")
        return ShipClass::Small;
    if (value == "medium")
        return ShipClass::Medium;
    if (value == "large")
        return ShipClass::Large;
    throw std::runtime_error(std::string("Lua rules: invalid ship class '") + value + "'.");
}

AccelerationMode readAcceleration(lua_State* state, int tableIndex, const char* key)
{
    const auto value = readString(state, tableIndex, key);
    if (value == "decelerate")
        return AccelerationMode::Decelerate;
    if (value == "cruise")
        return AccelerationMode::Cruise;
    if (value == "accelerate")
        return AccelerationMode::Accelerate;
    throw std::runtime_error(std::string("Lua rules: invalid acceleration mode '") + value + "'.");
}

void readShipDefaults(lua_State* state, int index, Ship& ship)
{
    ship.shipClass = readShipClass(state, index, "class");
    ship.turnRateDegreesPerSecond = readNumber(state, index, "turn_rate");
    const double throttleLevel = readNumber(state, index, "throttle_level");
    if (std::floor(throttleLevel) != throttleLevel || throttleLevel < 1.0 || throttleLevel > 4.0)
        throw std::runtime_error("Lua rules: ship throttle_level must be from 1 to 4.");
    ship.throttleLevel = static_cast<std::uint8_t>(throttleLevel);
    ship.collisionRadius = readNumber(state, index, "collision_radius");
    ship.velocity = readVector(state, index, "velocity");
    ship.orientation.yawDegrees = readNumber(state, index, "yaw");
    ship.orientation.pitchDegrees = readNumber(state, index, "pitch");
    ship.desiredOrientation.yawDegrees = readNumber(state, index, "desired_yaw");
    ship.desiredOrientation.pitchDegrees = readNumber(state, index, "desired_pitch");
    ship.speed = readNumber(state, index, "speed");
    ship.warp = readNumber(state, index, "warp");
    ship.acceleration = readAcceleration(state, index, "acceleration");
    ship.resources.energy = readNumber(state, index, "energy");
    ship.resources.rawMaterial = readNumber(state, index, "material");
    ship.combat.hull = readNumber(state, index, "hull");
    ship.combat.hullDamage = readNumber(state, index, "hull_damage");
    ship.combat.shields = readNumber(state, index, "shields");
    ship.combat.skill = readNumber(state, index, "skill");
    ship.combat.destroyed = readBoolean(state, index, "destroyed");
    ship.exists = true;
    if (ship.turnRateDegreesPerSecond < 0.0 || ship.throttleLevel == 0 ||
        ship.throttleLevel > 4 || ship.collisionRadius <= 0.0 || ship.speed < 0.0 ||
        ship.warp < 0.0 || ship.resources.energy < 0.0 || ship.resources.rawMaterial < 0.0 ||
        ship.combat.hull < 0.0 || ship.combat.hullDamage < 0.0 || ship.combat.shields < 0.0)
        throw std::runtime_error("Lua rules: invalid ship defaults.");
}

void requireDenseId(lua_State* state, int tableIndex, std::size_t expected)
{
    const double id = readNumber(state, tableIndex, "id");
    if (std::floor(id) != id || id != static_cast<double>(expected))
        throw std::runtime_error("Lua rules: scenario IDs must match their zero-based array slot.");
}

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
        firstEntityId >
            static_cast<double>(maxEntityId - kPracticeShipCount - kPlayerCapacity * 3) ||
        std::floor(firstPlayerId) != firstPlayerId || firstPlayerId < 0.0 ||
        firstPlayerId > static_cast<double>(kNoPlayer - kPlayerCapacity) ||
        std::floor(defaultWeapon) != defaultWeapon || defaultWeapon < 0.0 ||
        defaultWeapon >= kWeaponCount)
        throw std::runtime_error("Lua rules: invalid scenario ID settings.");
    rules.firstEntityId = static_cast<EntityId>(firstEntityId);
    rules.firstPlayerId = static_cast<PlayerId>(firstPlayerId);
    rules.defaultWeapon = static_cast<WeaponKind>(static_cast<std::size_t>(defaultWeapon));
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

    lua_getfield(state, scenario, "teams");
    requireTable(state, -1, "scenario.teams");
    for (std::size_t i = 0; i < rules.teams.size(); ++i)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(i + 1));
        requireTable(state, -1, "scenario.teams entry");
        requireDenseId(state, -1, i);
        rules.teams[i].id = static_cast<TeamId>(i);
        const double homePlanet = readNumber(state, -1, "home_planet_id");
        if (std::floor(homePlanet) != homePlanet || homePlanet < 0.0 ||
            homePlanet > std::numeric_limits<PlanetId>::max())
            throw std::runtime_error("Lua rules: team home_planet_id is out of range.");
        rules.teams[i].homePlanet = static_cast<PlanetId>(homePlanet);
        rules.teams[i].name = readString(state, -1, "name");
        rules.teams[i].shipClass = readShipClass(state, -1, "ship_class");
        const double startingOrbit = readNumber(state, -1, "starting_orbit_level");
        const double spawnRadius = readNumber(state, -1, "spawn_radius");
        if (std::floor(startingOrbit) != startingOrbit || startingOrbit < 0.0 ||
            startingOrbit > kMaxOrbitLevels || spawnRadius < 0.0)
            throw std::runtime_error("Lua rules: invalid team spawn or starting orbit.");
        rules.teams[i].startingOrbitLevel = static_cast<std::uint8_t>(startingOrbit);
        rules.teams[i].spawnRadius = spawnRadius;
        rules.teams[i].spawnOffset = readVector(state, -1, "spawn_offset");
        rules.teams[i].baseOffset = readVector(state, -1, "base_offset");
        rules.teams[i].spawnVerticalMinimum = readNumber(state, -1, "spawn_vertical_minimum");
        rules.teams[i].spawnVerticalMaximum = readNumber(state, -1, "spawn_vertical_maximum");
        rules.teams[i].randomSpawn = readBoolean(state, -1, "random_spawn");
        if (rules.teams[i].spawnVerticalMinimum < -1.0 ||
            rules.teams[i].spawnVerticalMaximum > 1.0 ||
            rules.teams[i].spawnVerticalMinimum >= rules.teams[i].spawnVerticalMaximum)
            throw std::runtime_error("Lua rules: invalid spawn vertical range.");
        lua_getfield(state, -1, "allied");
        requireTable(state, -1, "team.allied");
        const int allies = lua_absindex(state, -1);
        for (std::size_t ally = 0; ally < rules.teams[i].allied.size(); ++ally)
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
            if (!valid || allyId < 0 || allyId >= static_cast<lua_Integer>(kTeamCount))
                throw std::runtime_error("Lua rules: allied team ID is out of range.");
            rules.teams[i].allied[allyId] = true;
        }
        lua_pop(state, 1);
        lua_pop(state, 1);
    }
    lua_pop(state, 1);

    lua_getfield(state, scenario, "planets");
    requireTable(state, -1, "scenario.planets");
    const auto planetCount = lua_rawlen(state, -1);
    if (planetCount == 0 || planetCount > static_cast<std::size_t>(
                                               std::numeric_limits<PlanetId>::max()) + 1)
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
            planet.population < 0.0 ||
            planet.antiEntropy < 0.0 || planet.rawMaterial < 0.0 ||
            (controllingTeam != -1.0 &&
             (std::floor(controllingTeam) != controllingTeam || controllingTeam < 0.0 ||
              controllingTeam >= kTeamCount)) ||
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
    for (std::size_t i = 0; i < rules.weapons.size(); ++i)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(i + 1));
        requireTable(state, -1, "scenario.weapons entry");
        requireDenseId(state, -1, i);
        auto& weapon = rules.weapons[i];
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

    lua_getfield(state, scenario, "practice_ships");
    requireTable(state, -1, "scenario.practice_ships");
    for (std::size_t i = 0; i < rules.practiceNames.size(); ++i)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(i + 1));
        requireTable(state, -1, "scenario.practice_ships entry");
        const int targetIndex = lua_absindex(state, -1);
        const double teamId = readNumber(state, targetIndex, "team_id");
        if (teamId != static_cast<double>(i))
            throw std::runtime_error("Lua rules: practice ships must map to team slots in order.");
        rules.practiceNames[i] = readString(state, targetIndex, "name");
        rules.practicePositions[i] = readVector(state, targetIndex, "position");
        lua_pop(state, 1);
    }
    lua_pop(state, 3);
}

void throwLuaError(lua_State* state, const std::string& prefix)
{
    const std::string message =
        lua_tostring(state, -1) ? lua_tostring(state, -1) : "unknown Lua error";
    lua_pop(state, 1);
    throw std::runtime_error(prefix + message);
}

} // namespace

struct GameRules::Runtime
{
    lua_State* state{};
    int rulesReference{LUA_NOREF};
    int approachThrottleReference{LUA_NOREF};
    int weaponAimDirectionReference{LUA_NOREF};
    int orbitTransferStepReference{LUA_NOREF};

    ~Runtime()
    {
        if (state)
            lua_close(state);
    }
};

GameRules::GameRules(const std::filesystem::path& scriptPath)
    : runtime_(std::make_unique<Runtime>())
{
    runtime_->state = luaL_newstate();
    if (!runtime_->state)
        throw std::runtime_error("Could not create the Lua rules runtime.");

    openRuleLibraries(runtime_->state);
    if (luaL_dofile(runtime_->state, scriptPath.string().c_str()) != LUA_OK)
        throwLuaError(runtime_->state, "Could not load " + scriptPath.string() + ": ");

    lua_State* state = runtime_->state;
    requireTable(state, -1, "root");
    runtime_->rulesReference = luaL_ref(state, LUA_REGISTRYINDEX);
    runtime_->approachThrottleReference =
        cacheFunction(state, runtime_->rulesReference, "approach_throttle");
    runtime_->weaponAimDirectionReference =
        cacheFunction(state, runtime_->rulesReference, "weapon_aim_direction");
    runtime_->orbitTransferStepReference =
        cacheFunction(state, runtime_->rulesReference, "orbit_transfer_step");
    readScenario(state, runtime_->rulesReference, *this);

    lua_rawgeti(state, LUA_REGISTRYINDEX, runtime_->rulesReference);
    lua_getfield(state, -1, "orbit");
    requireTable(state, -1, "orbit");
    const double count = readNumber(state, -1, "level_count");
    if (std::floor(count) != count || count < 2.0 || count > kMaxOrbitLevels)
        throw std::runtime_error("Lua rules: orbit.level_count must be between 2 and 8.");
    levelCount = static_cast<std::uint8_t>(count);
    sphereOfInfluenceRadius = readNumber(state, -1, "sphere_of_influence_radius");
    if (sphereOfInfluenceRadius <= 0.0)
        throw std::runtime_error("Lua rules: sphere_of_influence_radius must be positive.");
    const double maxTransferSpeed = readNumber(state, -1, "transfer_max_radial_speed");
    const double transferAcceleration = readNumber(state, -1, "transfer_radial_acceleration");
    if (maxTransferSpeed <= 0.0 || transferAcceleration <= 0.0)
        throw std::runtime_error(
            "Lua rules: orbit transfer speed and acceleration must be positive.");

    lua_getfield(state, -1, "radius_fractions");
    requireTable(state, -1, "orbit.radius_fractions");
    for (std::uint8_t level = 1; level <= levelCount; ++level)
    {
        lua_geti(state, -1, level);
        const bool valid = lua_isnumber(state, -1);
        const double fraction = lua_tonumber(state, -1);
        lua_pop(state, 1);
        if (!valid || !std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0)
            throw std::runtime_error("Lua rules: radius fractions must be between 0 and 1.");
        if (level > 1 && fraction >= radiusFractions[level - 2])
            throw std::runtime_error("Lua rules: orbit fractions must decrease toward the planet.");
        radiusFractions[level - 1] = fraction;
    }
    if (radiusFractions[0] != 1.0)
        throw std::runtime_error("Lua rules: orbit level 1 must lie on the influence boundary.");
    lua_pop(state, 2); // radius_fractions, orbit
    lua_pop(state, 1); // root

    lua_rawgeti(state, LUA_REGISTRYINDEX, runtime_->rulesReference);
    lua_getfield(state, -1, "ship_classes");
    requireTable(state, -1, "ship_classes");
    constexpr const char* names[] = {"small", "medium", "large"};
    for (std::size_t index = 0; index < std::size(names); ++index)
    {
        lua_getfield(state, -1, names[index]);
        requireTable(state, -1, names[index]);
        const double maxLevel = readNumber(state, -1, "max_orbit_level");
        if (std::floor(maxLevel) != maxLevel || maxLevel < 1.0 || maxLevel > levelCount)
            throw std::runtime_error(std::string("Lua rules: invalid max_orbit_level for '") +
                                     names[index] + "'.");
        maximumLevelByClass[index] = static_cast<std::uint8_t>(maxLevel);
        lua_pop(state, 1);
    }
    for (const auto& team : teams)
        if (team.startingOrbitLevel > levelCount ||
            team.startingOrbitLevel > maximumLevelByClass[static_cast<std::size_t>(team.shipClass)])
            throw std::runtime_error("Lua rules: team starting orbit exceeds its ship limit.");
    if (playerDefaults.goalOrbitLevel > levelCount)
        throw std::runtime_error("Lua rules: player goal orbit exceeds the configured level count.");
    lua_pop(state, 2); // ship_classes, root

    lua_rawgeti(state, LUA_REGISTRYINDEX, runtime_->rulesReference);
    lua_getfield(state, -1, "flight");
    requireTable(state, -1, "flight");
    referenceGravitationalParameter = readNumber(state, -1, "reference_gravitational_parameter");
    thrustAcceleration = readNumber(state, -1, "thrust_acceleration");
    brakingAcceleration = readNumber(state, -1, "braking_acceleration");
    maximumAimLeadSeconds = readNumber(state, -1, "maximum_aim_lead_seconds");
    if (referenceGravitationalParameter <= 0.0 || thrustAcceleration <= 0.0 ||
        brakingAcceleration <= 0.0 || maximumAimLeadSeconds <= 0.0)
        throw std::runtime_error(
            "Lua rules: flight gravity and acceleration values must be positive.");
    lua_getfield(state, -1, "throttle_speed_multipliers");
    requireTable(state, -1, "flight.throttle_speed_multipliers");
    for (std::size_t index = 0; index < throttleSpeedMultipliers.size(); ++index)
    {
        lua_geti(state, -1, static_cast<lua_Integer>(index + 1));
        const bool valid = lua_isnumber(state, -1);
        const double multiplier = lua_tonumber(state, -1);
        lua_pop(state, 1);
        if (!valid || !std::isfinite(multiplier) || multiplier <= 0.0 ||
            (index > 0 && multiplier <= throttleSpeedMultipliers[index - 1]))
            throw std::runtime_error(
                "Lua rules: throttle speed multipliers must be positive and increasing.");
        throttleSpeedMultipliers[index] = multiplier;
    }
    lua_settop(state, 0);
}

GameRules::~GameRules() = default;
GameRules::GameRules(GameRules&&) noexcept = default;
GameRules& GameRules::operator=(GameRules&&) noexcept = default;

void GameRules::applyScenario(World& world) const
{
    if (world.nextEntityId < world.entityIdStart)
        throw std::runtime_error("World entity ID counter is below its configured start.");
    const std::uint64_t allocated = world.nextEntityId - world.entityIdStart;
    if (static_cast<std::uint64_t>(firstEntityId) + allocated >
        std::numeric_limits<EntityId>::max())
        throw std::runtime_error("Lua rules: entity ID range overflows this world.");
    const auto offset = static_cast<std::int64_t>(firstEntityId) - world.entityIdStart;

    std::array<PlayerId, kPlayerCapacity> previousPlayerIds{};
    for (std::size_t i = 0; i < world.playerSlots.size(); ++i)
        if (world.playerSlots[i])
            previousPlayerIds[i] = world.playerSlots[i]->id;

    auto shiftEntityId = [offset, &world](EntityId& id)
    {
        if (id < world.entityIdStart)
            throw std::runtime_error("World contains an entity ID below its configured start.");
        id = static_cast<EntityId>(static_cast<std::int64_t>(id) + offset);
    };
    for (auto& slot : world.ships)
        if (slot)
            shiftEntityId(slot->id);
    for (auto& slot : world.bases)
        if (slot)
            shiftEntityId(slot->id);
    for (auto& slot : world.torpedoes)
        if (slot)
            shiftEntityId(slot->id);

    for (std::size_t i = 0; i < world.playerSlots.size(); ++i)
    {
        if (!world.playerSlots[i])
            continue;
        auto& player = *world.playerSlots[i];
        player.id = firstPlayerId + static_cast<PlayerId>(i);
        shiftEntityId(player.shipId);
        shiftEntityId(player.baseId);
        shiftEntityId(player.torpedoId);
        shiftEntityId(player.selectedObject);
        if (player.selectedTarget)
            shiftEntityId(*player.selectedTarget);
        if (player.goal && player.goal->kind == Target::Kind::Entity)
            shiftEntityId(player.goal->id);
    }

    auto remapOwner = [&](PlayerId& owner)
    {
        for (std::size_t i = 0; i < previousPlayerIds.size(); ++i)
            if (world.playerSlots[i] && owner == previousPlayerIds[i])
            {
                owner = firstPlayerId + static_cast<PlayerId>(i);
                return;
            }
    };
    for (auto& slot : world.ships)
        if (slot)
            remapOwner(slot->owner);
    for (auto& slot : world.bases)
        if (slot)
            remapOwner(slot->owner);
    for (auto& slot : world.torpedoes)
        if (slot)
            remapOwner(slot->owner);

    world.nextEntityId = static_cast<EntityId>(firstEntityId + allocated);
    world.entityIdStart = firstEntityId;
    world.playerIdStart = firstPlayerId;
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
    for (std::size_t i = 0; i < kPracticeShipCount; ++i)
    {
        if (!world.ships[i])
            continue;
        const auto id = world.ships[i]->id;
        auto& target = *world.ships[i];
        target = practiceShipDefaults;
        target.id = id;
        target.owner = kNoPlayer;
        target.team = teams[i].id;
        target.name = practiceNames[i];
        target.position = practicePositions[i];
    }
}

std::uint8_t GameRules::maximumLevel(ShipClass shipClass) const noexcept
{
    const auto index = static_cast<std::size_t>(shipClass);
    return index < maximumLevelByClass.size() ? maximumLevelByClass[index] : 1;
}

double GameRules::orbitRadius(const Planet& planet, std::uint8_t level) const noexcept
{
    if (level == 0 || level > levelCount)
        return 0.0;
    const double innerRadius = planet.radius + planet.parkingOrbitAltitude;
    return innerRadius +
           (planet.sphereOfInfluenceRadius - innerRadius) * radiusFractions[level - 1];
}

double GameRules::speedLimit(std::uint8_t throttleLevel,
                             double gravitationalParameter,
                             double sphereOfInfluenceRadius) const noexcept
{
    if (throttleLevel == 0 || throttleLevel > throttleSpeedMultipliers.size() ||
        gravitationalParameter <= 0.0 || sphereOfInfluenceRadius <= 0.0)
        return 0.0;
    const double referenceSpeed = std::sqrt(gravitationalParameter / sphereOfInfluenceRadius);
    return referenceSpeed * throttleSpeedMultipliers[throttleLevel - 1];
}

AccelerationMode GameRules::approachThrottle(double distanceToCapture,
                                             double currentSpeed,
                                             double closingSpeed,
                                             double maximumSpeed,
                                             double deltaSeconds) const
{
    lua_State* state = runtime_->state;
    lua_rawgeti(state, LUA_REGISTRYINDEX, runtime_->approachThrottleReference);
    lua_pushnumber(state, distanceToCapture);
    lua_pushnumber(state, currentSpeed);
    lua_pushnumber(state, closingSpeed);
    lua_pushnumber(state, maximumSpeed);
    lua_pushnumber(state, thrustAcceleration);
    lua_pushnumber(state, brakingAcceleration);
    lua_pushnumber(state, deltaSeconds);
    if (lua_pcall(state, 7, 1, 0) != LUA_OK)
        throwLuaError(state, "Lua approach_throttle failed: ");
    const bool valid = lua_isinteger(state, -1);
    const lua_Integer action = lua_tointeger(state, -1);
    lua_settop(state, 0);
    if (!valid || action < -1 || action > 1)
        throw std::runtime_error("Lua rules: approach_throttle must return -1, 0, or 1.");
    if (action < 0)
        return AccelerationMode::Decelerate;
    if (action > 0)
        return AccelerationMode::Accelerate;
    return AccelerationMode::Cruise;
}

Vec3 GameRules::weaponAimDirection(const Vec3& relativePosition,
                                   const Vec3& relativeVelocity,
                                   double projectileSpeed) const
{
    lua_State* state = runtime_->state;
    lua_rawgeti(state, LUA_REGISTRYINDEX, runtime_->weaponAimDirectionReference);
    lua_pushnumber(state, relativePosition.x);
    lua_pushnumber(state, relativePosition.y);
    lua_pushnumber(state, relativePosition.z);
    lua_pushnumber(state, relativeVelocity.x);
    lua_pushnumber(state, relativeVelocity.y);
    lua_pushnumber(state, relativeVelocity.z);
    lua_pushnumber(state, projectileSpeed);
    lua_pushnumber(state, maximumAimLeadSeconds);
    if (lua_pcall(state, 8, 3, 0) != LUA_OK)
        throwLuaError(state, "Lua weapon_aim_direction failed: ");

    const bool valid =
        lua_isnumber(state, -3) && lua_isnumber(state, -2) && lua_isnumber(state, -1);
    const Vec3 direction{lua_tonumber(state, -3), lua_tonumber(state, -2), lua_tonumber(state, -1)};
    lua_settop(state, 0);
    const double length = direction.length();
    if (!valid || !std::isfinite(length) || length <= 1e-9)
        throw std::runtime_error("Lua rules: weapon_aim_direction must return a non-zero vector.");
    return direction * (1.0 / length);
}

double
GameRules::transferRadialStep(double currentRadius, double targetRadius, double deltaSeconds) const
{
    lua_State* state = runtime_->state;
    lua_rawgeti(state, LUA_REGISTRYINDEX, runtime_->orbitTransferStepReference);
    lua_pushnumber(state, currentRadius);
    lua_pushnumber(state, targetRadius);
    lua_pushnumber(state, deltaSeconds);
    if (lua_pcall(state, 3, 1, 0) != LUA_OK)
        throwLuaError(state, "Lua orbit_transfer_step failed: ");
    const bool valid = lua_isnumber(state, -1);
    const double step = lua_tonumber(state, -1);
    lua_settop(state, 0);
    if (!valid || !std::isfinite(step))
        throw std::runtime_error("Lua rules: orbit_transfer_step must return a finite number.");
    return step;
}

} // namespace spasim
