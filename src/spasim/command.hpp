#pragma once

#include "spasim/model.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace spasim {

enum class CommandKind : std::uint8_t {
    Help, Status, Teams, Planets, Tick, Turn, Accelerate, Brake,
    SetGoal, Fire, Quit, Unknown
};

struct Command {
    CommandKind kind{CommandKind::Unknown};
    std::vector<std::string> arguments;
};

[[nodiscard]] Command parseCommand(std::string_view line);

} // namespace spasim
