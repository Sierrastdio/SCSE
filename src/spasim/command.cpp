#include "spasim/command.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <algorithm>
#include <initializer_list>
#include <sstream>
#include <system_error>

namespace spasim
{
namespace
{
std::string lower(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool parseEntityIdToken(std::string token, std::uint32_t& id)
{
    token = lower(std::move(token));
    if (token.starts_with("ship-"))
        token.erase(0, 5);
    else if (token.starts_with('t') && token.size() > 1)
        token.erase(0, 1);
    if (token.starts_with('#'))
        token.erase(0, 1);
    if (token.empty())
        return false;
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), id);
    return error == std::errc{} && end == token.data() + token.size();
}
} // namespace

Command parseCommand(std::string_view line)
{
    std::istringstream input{std::string(line)};
    std::string name;
    input >> name;
    name = lower(std::move(name));

    Command result;
    if (name == "help" || name == "?")
        result.kind = CommandKind::Help;
    else if (name == "status" || name == "s")
        result.kind = CommandKind::Status;
    else if (name == "teams")
        result.kind = CommandKind::Teams;
    else if (name == "planets")
        result.kind = CommandKind::Planets;
    else if (name == "tick" || name == "wait")
        result.kind = CommandKind::Tick;
    else if (name == "turn")
        result.kind = CommandKind::Turn;
    else if (name == "accelerate" || name == "accel")
        result.kind = CommandKind::Accelerate;
    else if (name == "brake" || name == "decel")
        result.kind = CommandKind::Brake;
    else if (name == "cruise" || name == "coast")
        result.kind = CommandKind::Cruise;
    else if (name == "throttle")
        result.kind = CommandKind::SetThrottleLevel;
    else if (name == "goal")
        result.kind = CommandKind::SetGoal;
    else if (name == "orbit")
        result.kind = CommandKind::SetOrbitLevel;
    else if (name == "class")
        result.kind = CommandKind::SetShipClass;
    else if (name == "select" || name == "target")
        result.kind = CommandKind::Select;
    else if (name == "fire" || name == "shoot")
        result.kind = CommandKind::Fire;
    else if (name == "torpedo" || name == "launch")
        result.kind = CommandKind::FireTorpedo;
    else if (name == "detonate")
        result.kind = CommandKind::Detonate;
    else if (name == "weapon")
        result.kind = CommandKind::SetWeapon;
    else if (name == "repair")
        result.kind = CommandKind::Repair;
    else if (name == "transfer")
        result.kind = CommandKind::Transfer;
    else if (name == "load")
        result.kind = CommandKind::Load;
    else if (name == "ally")
        result.kind = CommandKind::Ally;
    else if (name == "pause")
        result.kind = CommandKind::Pause;
    else if (name == "resume")
        result.kind = CommandKind::Resume;
    else if (name == "speed")
        result.kind = CommandKind::Speed;
    else if (name == "snapshot")
        result.kind = CommandKind::Snapshot;
    else if (name == "quit" || name == "exit")
        result.kind = CommandKind::Quit;
    std::string first;
    std::string second;
    std::string third;
    std::string fourth;
    input >> first;
    input >> second;
    input >> third;
    input >> fourth;
    result.text = first;
    auto parse = [](const std::string& text, double& value)
    {
        if (text.empty())
            return false;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        return error == std::errc{} && end == text.data() + text.size();
    };
    if (result.kind == CommandKind::Turn)
    {
        result.hasFirst = parse(first, result.first);
        result.hasSecond = parse(second, result.second);
        result.hasThird = parse(third, result.thirdValue);
    }
    else if (result.kind == CommandKind::Ally)
    {
        result.hasFirst = parse(first, result.first);
        result.hasSecond = parse(second, result.second);
    }
    else if (result.kind == CommandKind::SetGoal)
    {
        double target{};
        result.hasTarget =
            parse(first, target) && target >= 1.0 && target <= 4.0 && std::floor(target) == target;
        if (result.hasTarget)
            result.target = static_cast<std::uint32_t>(target - 1.0);
        if (!second.empty())
        {
            result.hasSecond = true;
            parse(second, result.second);
        }
    }
    else if (result.kind == CommandKind::SetOrbitLevel)
    {
        double level{};
        result.hasTarget = parse(first, level) && level >= 1.0 && level <= kMaxOrbitLevels &&
                           std::floor(level) == level;
        if (result.hasTarget)
            result.target = static_cast<std::uint32_t>(level);
    }
    else if (result.kind == CommandKind::Fire)
    {
        result.hasFirst = parse(first, result.first);
        if (!result.hasFirst && first.empty())
        {
            result.first = 1.0;
            result.hasFirst = true;
        }
    }
    else if (result.kind == CommandKind::FireTorpedo || result.kind == CommandKind::Repair)
    {
        result.hasFirst = parse(first, result.first);
    }
    else if (result.kind == CommandKind::Select)
    {
        for (const auto& token : {first, second, third, fourth})
        {
            if (token.empty() || lower(token) == "ship")
                continue;
            if (parseEntityIdToken(token, result.target))
            {
                result.hasTarget = true;
                break;
            }
        }
    }
    else if (result.kind == CommandKind::SetWeapon)
    {
        double weapon{};
        result.hasTarget =
            parse(first, weapon) && weapon >= 1.0 && weapon <= 4.0 && std::floor(weapon) == weapon;
        if (result.hasTarget)
            result.target = static_cast<std::uint32_t>(weapon - 1.0);
    }
    else if (result.kind == CommandKind::Transfer || result.kind == CommandKind::Load)
    {
        result.text = first;
        result.hasFirst = parse(second, result.first);
        try
        {
            result.target = static_cast<std::uint32_t>(std::stoul(third));
            result.hasTarget = true;
        }
        catch (...)
        {
            result.hasTarget = false;
        }
    }
    else if (result.kind == CommandKind::Snapshot)
    {
        result.text = first;
    }
    else if (result.kind == CommandKind::Speed)
    {
        result.hasFirst = parse(first, result.first);
    }
    else if (result.kind == CommandKind::SetThrottleLevel)
    {
        double level{};
        result.hasTarget =
            parse(first, level) && level >= 1.0 && level <= 4.0 && std::floor(level) == level;
        if (result.hasTarget)
            result.target = static_cast<std::uint32_t>(level);
    }
    return result;
}

} // namespace spasim
