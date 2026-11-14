#pragma once

#include "engine/types.hpp"

#include <string_view>
#include <vector>

namespace spasim
{

enum class CommandKind : std::uint8_t
{
  Help,
  Status,
  Teams,
  Planets,
  Tick,
  Turn,
  Accelerate,
  Brake,
  Cruise,
  SetThrottleLevel,
  SetGoal,
  SetOrbitLevel,
  SetShipClass,
  Target,
  SelectShips,
  Fire,
  FireTorpedo,
  Detonate,
  SetWeapon,
  Repair,
  Transfer,
  Load,
  Ally,
  Pause,
  Resume,
  Speed,
  Snapshot,
  Quit,
  Unknown
};

struct Command
{
  CommandKind kind{CommandKind::Unknown};

  double first{};
  double second{};
  double thirdValue{};

  std::uint32_t target{};
  std::string text;
  std::vector<EntityId> shipIds;

  bool hasFirst{};
  bool hasSecond{};
  bool hasThird{};

  bool hasTarget{};
  bool hasInvalidShipId{};
};

[[nodiscard]] Command parseCommand(std::string_view line);

} // namespace spasim
