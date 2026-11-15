#pragma once

#include "engine/actions.hpp"
#include "engine/types.hpp"
#include "rules/game_rules.hpp"

namespace spasim
{

class Simulation
{
public:
  explicit Simulation(World world);

  [[nodiscard]] World &world() noexcept { return world_; }
  [[nodiscard]] const World &world() const noexcept { return world_; }
  [[nodiscard]] const GameRules &rules() const noexcept { return gameRules_; }
  void step();
  [[nodiscard]] CommandResult execute(PlayerId playerId, const WorldCommand &command);

private:
  World world_;
  GameRules gameRules_;
};

} // namespace spasim
