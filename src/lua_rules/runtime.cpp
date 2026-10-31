#include "lua_rules/lua_rules.hpp"
#include "lua_rules/internal.hpp"

#include <lua.hpp>

#include <cmath>
#include <stdexcept>
#include <string>

namespace spasim
{
using namespace lua_detail;

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
        throw std::runtime_error(
            "Lua rules: player goal orbit exceeds the configured level count.");
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
    const auto playerCapacity = teams.size() * playersPerTeam;
    const auto entityCapacity = playerCapacity + practiceNames.size();
    world.nextEntityId = firstEntityId;
    world.entityIdStart = firstEntityId;
    world.playerIdStart = firstPlayerId;
    world.playersPerTeam = playersPerTeam;
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
    world.ships.resize(entityCapacity);
    world.bases.resize(entityCapacity);
    world.torpedoes.resize(entityCapacity);
    for (std::size_t i = 0; i < practiceNames.size(); ++i)
    {
        Ship target = practiceShipDefaults;
        target.id = world.nextEntityId++;
        target.owner = kNoPlayer;
        target.team = practiceTeams[i];
        target.playerControlled = false;
        target.name = practiceNames[i];
        target.position = practicePositions[i];
        world.ships[i] = std::move(target);
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



