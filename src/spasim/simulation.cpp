#include "spasim/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <sstream>

namespace spasim {
namespace {
double parseNumber(const std::vector<std::string>& args, std::size_t index, double fallback) {
    if (index >= args.size()) return fallback;
    try { return std::stod(args[index]); }
    catch (...) { return fallback; }
}

Vec3 forwardVector(const Orientation& orientation) {
    const double radians = std::numbers::pi / 180.0;
    const double yaw = orientation.yawDegrees * radians;
    const double pitch = orientation.pitchDegrees * radians;
    return {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
}
} // namespace

Simulation::Simulation(World world) : world_(std::move(world)) {}

void Simulation::step() {
    updateMovement();
    updateTorpedoes();
    updatePlanetEconomy();
    world_.elapsedSeconds += world_.fixedStepSeconds;
    ++world_.tick;
}

std::string Simulation::execute(PlayerId playerId, const Command& command) {
    auto* player = world_.findPlayer(playerId);
    if (!player) return "No active player in that slot.";
    auto* ship = world_.findShip(player->shipId);
    if (!ship) return "Your ship is unavailable.";

    std::ostringstream message;
    switch (command.kind) {
    case CommandKind::Turn:
        if (command.arguments.size() < 2) return "Usage: turn <yaw-degrees> <pitch-degrees>";
        ship->orientation.yawDegrees = std::remainder(parseNumber(command.arguments, 0, 0.0), 360.0);
        ship->orientation.pitchDegrees = std::clamp(parseNumber(command.arguments, 1, 0.0), -90.0, 90.0);
        message << "Heading set to yaw " << ship->orientation.yawDegrees << "°, pitch "
                << ship->orientation.pitchDegrees << "°.";
        return message.str();
    case CommandKind::Accelerate:
        ship->acceleration = AccelerationMode::Accelerate;
        return "Acceleration set to accelerate.";
    case CommandKind::Brake:
        ship->acceleration = AccelerationMode::Decelerate;
        return "Acceleration set to decelerate.";
    case CommandKind::SetGoal: {
        if (command.arguments.empty()) return "Usage: goal <planet-number>";
        const auto value = static_cast<int>(parseNumber(command.arguments, 0, -1.0));
        if (value < 1 || value > static_cast<int>(kPlanetCount)) return "Planet must be 1–4.";
        player->goal.reset();
        // Planet goals use a separate optional in the first implementation;
        // position is resolved by movement while this marker is exposed in status.
        player->selectedObject = static_cast<EntityId>(value);
        return "Goal set to " + world_.planets[static_cast<std::size_t>(value - 1)].name + ".";
    }
    case CommandKind::Fire: {
        if (!player->phasorEnabled) return "Phasor is not energized. Use fire <power> to energize and fire.";
        const double power = std::clamp(parseNumber(command.arguments, 0, player->phasorPower), 0.0,
                                        std::min(player->phasorPower, ship->resources.energy));
        ship->resources.energy -= power;
        player->phasorEnabled = false;
        return "Phasor discharge launched (" + std::to_string(power) + " power). Hit resolution is pending formula recovery.";
    }
    case CommandKind::Tick:
        step();
        return "Simulation advanced one fixed step.";
    case CommandKind::Status:
    case CommandKind::Teams:
    case CommandKind::Planets:
    case CommandKind::Help:
    case CommandKind::Quit:
    case CommandKind::Unknown:
        return "This command is handled by the console.";
    }
    return "Unknown command.";
}

void Simulation::updateMovement() {
    for (auto& ship : world_.ships) {
        if (!ship.exists || ship.combat.destroyed || ship.resources.energy <= 0.0) continue;
        const double acceleration = ship.acceleration == AccelerationMode::Accelerate ? 1.0
                                  : ship.acceleration == AccelerationMode::Decelerate ? -1.0 : 0.0;
        ship.warp = std::max(0.0, ship.warp + acceleration * world_.fixedStepSeconds);
        ship.speed = ship.warp;
        ship.position += forwardVector(ship.orientation) * (ship.speed * world_.fixedStepSeconds);
        if (ship.acceleration != AccelerationMode::Cruise) {
            ship.resources.energy = std::max(0.0, ship.resources.energy - std::abs(acceleration) * world_.fixedStepSeconds);
        }
    }
}

void Simulation::updateTorpedoes() {
    for (auto& torpedo : world_.torpedoes) {
        if (!torpedo.exists || torpedo.detonated) continue;
        torpedo.position += torpedo.velocity * world_.fixedStepSeconds;
    }
}

void Simulation::updatePlanetEconomy() {
    // Economy equations remain intentionally inactive until the printed
    // legacy operators and common-memory initial constants are decoded.
}

} // namespace spasim
