#include "spasim/command.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace spasim {
namespace {
std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}
} // namespace

Command parseCommand(std::string_view line) {
    std::istringstream input{std::string(line)};
    std::string name;
    input >> name;
    name = lower(std::move(name));

    Command result;
    if (name == "help" || name == "?") result.kind = CommandKind::Help;
    else if (name == "status" || name == "s") result.kind = CommandKind::Status;
    else if (name == "teams") result.kind = CommandKind::Teams;
    else if (name == "planets") result.kind = CommandKind::Planets;
    else if (name == "tick" || name == "wait") result.kind = CommandKind::Tick;
    else if (name == "turn") result.kind = CommandKind::Turn;
    else if (name == "accelerate" || name == "accel") result.kind = CommandKind::Accelerate;
    else if (name == "brake" || name == "decel") result.kind = CommandKind::Brake;
    else if (name == "goal") result.kind = CommandKind::SetGoal;
    else if (name == "fire" || name == "shoot") result.kind = CommandKind::Fire;
    else if (name == "quit" || name == "exit") result.kind = CommandKind::Quit;
    while (input) {
        std::string argument;
        if (!(input >> argument)) break;
        result.arguments.push_back(std::move(argument));
    }
    return result;
}

} // namespace spasim
