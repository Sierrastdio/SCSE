#pragma once

#include "lua_rules/lua_rules.hpp"

#include <lua.hpp>

#include <string>

namespace spasim::lua_detail
{
double readNumber(lua_State* state, int tableIndex, const char* key);
void requireTable(lua_State* state, int index, const char* description);
int cacheFunction(lua_State* state, int rulesReference, const char* name);
void openRuleLibraries(lua_State* state);
std::string readString(lua_State* state, int tableIndex, const char* key);
Vec3 readVector(lua_State* state, int tableIndex, const char* key);
bool readBoolean(lua_State* state, int tableIndex, const char* key);
ShipClass readShipClass(lua_State* state, int tableIndex, const char* key);
AccelerationMode readAcceleration(lua_State* state, int tableIndex, const char* key);
void readShipDefaults(lua_State* state, int index, Ship& ship);
void requireDenseId(lua_State* state, int tableIndex, std::size_t expected);
void readScenario(lua_State* state, int rulesReference, GameRules& rules);
[[noreturn]] void throwLuaError(lua_State* state, const std::string& prefix);
} // namespace spasim::lua_detail


