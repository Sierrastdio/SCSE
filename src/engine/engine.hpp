#pragma once

#include "engine/actions.hpp"
#include "engine/types.hpp"
#include "lua_rules/lua_rules.hpp"

#include <filesystem>

namespace spasim
{

class Simulation
{
  public:
    explicit Simulation(World world, std::filesystem::path rulesPath = "scripts/game_rules.lua");

    [[nodiscard]] World& world() noexcept
    {
        return world_;
    }
    [[nodiscard]] const World& world() const noexcept
    {
        return world_;
    }
    void step();
    [[nodiscard]] CommandResult execute(PlayerId playerId, const WorldCommand& command);

  private:
    World world_;
    GameRules gameRules_;
};

} // namespace spasim


