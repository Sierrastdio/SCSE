#include "lua_rules/internal.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace spasim::lua_detail
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
    if (ship.turnRateDegreesPerSecond < 0.0 || ship.throttleLevel == 0 || ship.throttleLevel > 4 ||
        ship.collisionRadius <= 0.0 || ship.speed < 0.0 || ship.warp < 0.0 ||
        ship.resources.energy < 0.0 || ship.resources.rawMaterial < 0.0 || ship.combat.hull < 0.0 ||
        ship.combat.hullDamage < 0.0 || ship.combat.shields < 0.0)
        throw std::runtime_error("Lua rules: invalid ship defaults.");
}

void requireDenseId(lua_State* state, int tableIndex, std::size_t expected)
{
    const double id = readNumber(state, tableIndex, "id");
    if (std::floor(id) != id || id != static_cast<double>(expected))
        throw std::runtime_error("Lua rules: scenario IDs must match their zero-based array slot.");
}

void throwLuaError(lua_State* state, const std::string& prefix)
{
    const std::string message =
        lua_tostring(state, -1) ? lua_tostring(state, -1) : "unknown Lua error";
    lua_pop(state, 1);
    throw std::runtime_error(prefix + message);
}
} // namespace spasim::lua_detail



