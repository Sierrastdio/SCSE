#pragma once

#include "spasim/model.hpp"

#include <string_view>

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
    Select,
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
    bool hasFirst{};
    bool hasSecond{};
    bool hasThird{};
    bool hasTarget{};
};

[[nodiscard]] Command parseCommand(std::string_view line);

} // namespace spasim
