#include "spasim/simulation_detail.hpp"

#include <cmath>

namespace spasim::detail {

CommandResult executeAction(World&, Player& player, Ship& ship, const SetHeading& action) {
    if (!std::isfinite(action.direction.x) || !std::isfinite(action.direction.y) ||
        !std::isfinite(action.direction.z) || action.direction.length() <= 1e-9)
        return {CommandStatus::Invalid, "Direction vector must be finite and non-zero."};

    player.goal.reset();
    ship.desiredOrientation = orientationToward(action.direction);
    if (ship.acceleration == AccelerationMode::Accelerate)
        return {CommandStatus::Applied, "Facing set; the ship will thrust along it as it turns."};
    if (ship.acceleration == AccelerationMode::Decelerate)
        return {CommandStatus::Applied, "Facing set; braking still opposes current velocity. Use accelerate to thrust along the new facing."};
    return {CommandStatus::Applied, "Facing set; ship is coasting. Use accelerate to thrust along this direction."};
}

CommandResult executeAction(World&, Player&, Ship& ship, const SetAcceleration& action) {
    ship.acceleration = action.mode;
    if (action.mode != AccelerationMode::Cruise) ship.orbitingPlanet.reset();
    switch (action.mode) {
    case AccelerationMode::Accelerate:
        return {CommandStatus::Applied, "Engines applying thrust along the ship's facing."};
    case AccelerationMode::Decelerate:
        return {CommandStatus::Applied, "Braking against current velocity."};
    case AccelerationMode::Cruise:
        return {CommandStatus::Applied, "Coasting; gravity and momentum still move the ship."};
    }
    return {CommandStatus::Invalid, "Unknown acceleration mode."};
}

CommandResult executeAction(World& world, Player& player, Ship& ship, const SetGoal& action) {
    auto* destination = positionOf(world, action.target);
    if (!destination) return {CommandStatus::NotFound, "Goal target not found."};

    player.goal = action.target;
    player.selectedTarget.reset();
    ship.orbitingPlanet.reset();
    const Vec3 direction = *destination - ship.position;
    if (direction.length() > 1e-9) ship.desiredOrientation = orientationToward(direction);

    return {CommandStatus::Applied,
            "Navigation set. Use accelerate to travel; reaching the planet's orbit boundary inserts the ship into orbit."};
}

CommandResult executeAction(World& world, Player& player, Ship& ship, const SelectTarget& action) {
    if (action.target.kind != Target::Kind::Entity)
        return {CommandStatus::Invalid, "Select requires an entity target."};

    auto* targetPosition = positionOf(world, action.target);
    if (!targetPosition) return {CommandStatus::NotFound, "Selectable entity not found."};

    const Vec3 direction = *targetPosition - ship.position;
    if (direction.length() <= 1e-9)
        return {CommandStatus::Invalid, "Cannot aim at an entity at your exact position."};

    player.selectedTarget = action.target.id;
    player.goal.reset();
    ship.desiredOrientation = orientationToward(direction);
    return {CommandStatus::Applied, "Target selected; ship is aiming at it."};
}

} // namespace spasim::detail
