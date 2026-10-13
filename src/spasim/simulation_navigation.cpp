#include "spasim/simulation_detail.hpp"

#include <cmath>

namespace spasim::detail
{

CommandResult executeAction(World&, Player& player, Ship& ship, const SetHeading& action)
{
    if (!std::isfinite(action.direction.x) || !std::isfinite(action.direction.y) ||
        !std::isfinite(action.direction.z) || action.direction.length() <= 1e-9)
        return {CommandStatus::Invalid, "Direction vector must be finite and non-zero."};

    player.goal.reset();
    if (ship.orbitTransferTargetLevel)
    {
        ship.orbitTransferTargetLevel.reset();
        ship.orbitingPlanet.reset();
        ship.orbitLevel = 0;
    }
    ship.desiredOrientation = orientationToward(action.direction);
    if (ship.acceleration == AccelerationMode::Accelerate)
        return {CommandStatus::Applied, "Facing set; the ship will thrust along it as it turns."};
    if (ship.acceleration == AccelerationMode::Decelerate)
        return {CommandStatus::Applied,
                "Facing set; braking still opposes current velocity. Use accelerate to thrust "
                "along the new facing."};
    return {CommandStatus::Applied,
            "Facing set; ship is coasting. Use accelerate to thrust along this direction."};
}

CommandResult executeAction(World&, Player&, Ship& ship, const SetAcceleration& action)
{
    const bool wasTransferringOrbit = ship.orbitTransferTargetLevel.has_value();
    ship.acceleration = action.mode;
    ship.orbitTransferTargetLevel.reset();
    if (action.mode != AccelerationMode::Cruise)
    {
        ship.orbitingPlanet.reset();
        ship.orbitLevel = 0;
    }
    else if (wasTransferringOrbit)
    {
        ship.orbitingPlanet.reset();
        ship.orbitLevel = 0;
    }
    switch (action.mode)
    {
    case AccelerationMode::Accelerate:
        return {CommandStatus::Applied, "Engines applying thrust along the ship's facing."};
    case AccelerationMode::Decelerate:
        return {CommandStatus::Applied, "Braking against current velocity."};
    case AccelerationMode::Cruise:
        return {CommandStatus::Applied, "Coasting; gravity and momentum still move the ship."};
    }
    return {CommandStatus::Invalid, "Unknown acceleration mode."};
}

CommandResult executeAction(World&, Player&, Ship& ship, const SetThrottleLevel& action)
{
    if (action.level == 0 || action.level > 4)
        return {CommandStatus::Invalid, "Throttle stage must be from 1 to 4."};
    ship.throttleLevel = action.level;
    return {CommandStatus::Applied, "Throttle stage set to " + std::to_string(action.level) + "."};
}

CommandResult executeAction(
    World& world, Player& player, Ship& ship, const SetGoal& action, const GameRules& rules)
{
    auto* destination = positionOf(world, action.target);
    if (!destination)
        return {CommandStatus::NotFound, "Goal target not found."};
    if (action.target.kind != Target::Kind::Planet || action.target.id >= world.planets.size())
        return {CommandStatus::Invalid, "Navigation goals must target a planet."};
    if (action.orbitLevel == 0 || action.orbitLevel > rules.maximumLevel(ship.shipClass))
        return {CommandStatus::Invalid,
                "This ship class cannot enter that orbit level; check class and Lua orbit rules."};

    // A goal for the planet we already orbit is an orbit-level order. Keep the
    // current orbital state and transfer from its present radius and velocity.
    if (ship.orbitingPlanet && *ship.orbitingPlanet == action.target.id)
    {
        player.goal.reset();
        player.goalOrbitLevel = action.orbitLevel;
        player.selectedTarget.reset();
        if (ship.orbitLevel == action.orbitLevel && !ship.orbitTransferTargetLevel)
            return {CommandStatus::Applied, "Ship is already at that orbit level."};

        ship.orbitTransferTargetLevel = action.orbitLevel;
        ship.acceleration = AccelerationMode::Cruise;
        return {CommandStatus::Applied,
                "Orbit autopilot engaged for level " + std::to_string(action.orbitLevel) +
                    " from the current orbit."};
    }

    player.goal = action.target;
    player.goalOrbitLevel = action.orbitLevel;
    player.selectedTarget.reset();
    ship.orbitingPlanet.reset();
    ship.orbitLevel = 0;
    ship.orbitTransferTargetLevel.reset();
    // A navigation goal is an autopilot command: it must keep applying thrust
    // even when the ship was coasting after an orbit insertion or cruise order.
    ship.acceleration = AccelerationMode::Accelerate;
    const Vec3 direction = *destination - ship.position;
    if (direction.length() > 1e-9)
        ship.desiredOrientation = orientationToward(direction);

    return {CommandStatus::Applied,
            "Autopilot engaged: approaching the planet, then entering orbit level " +
                std::to_string(action.orbitLevel) + "."};
}

CommandResult executeAction(
    World& world, Player&, Ship& ship, const SetOrbitLevel& action, const GameRules& rules)
{
    if (!ship.orbitingPlanet || *ship.orbitingPlanet >= world.planets.size())
        return {CommandStatus::Invalid, "The ship must be in orbit before changing orbit level."};
    if (action.level == 0 || action.level > rules.maximumLevel(ship.shipClass))
        return {CommandStatus::Invalid,
                "This ship class cannot enter that orbit level; check class and Lua orbit rules."};
    if (action.level == ship.orbitLevel)
        return {CommandStatus::Applied, "Ship is already at that orbit level."};

    ship.orbitTransferTargetLevel = action.level;
    ship.acceleration = AccelerationMode::Accelerate;
    return {CommandStatus::Applied,
            "Orbit autopilot engaged for level " + std::to_string(action.level) + "."};
}

CommandResult
executeAction(World&, Player&, Ship& ship, const SetShipClass& action, const GameRules& rules)
{
    const auto maxLevel = rules.maximumLevel(action.shipClass);
    if (ship.orbitLevel > maxLevel ||
        (ship.orbitTransferTargetLevel && *ship.orbitTransferTargetLevel > maxLevel))
        return {CommandStatus::Invalid,
                "Cannot change class while occupying an orbit level that class cannot reach."};
    ship.shipClass = action.shipClass;
    return {CommandStatus::Applied,
            std::string("Ship class set to ") + toString(ship.shipClass) + "."};
}

CommandResult executeAction(
    World& world, Player& player, Ship& ship, const SelectTarget& action, const GameRules& rules)
{
    if (action.target.kind != Target::Kind::Entity)
        return {CommandStatus::Invalid, "Select requires an entity target."};

    auto* targetPosition = positionOf(world, action.target);
    if (!targetPosition)
        return {CommandStatus::NotFound, "Selectable entity not found."};

    const Vec3 relativePosition = *targetPosition - ship.position;
    if (relativePosition.length() <= 1e-9)
        return {CommandStatus::Invalid, "Cannot aim at an entity at your exact position."};
    Vec3 targetVelocity{};
    if (const auto* targetShip = world.findShip(action.target.id))
        targetVelocity = targetShip->velocity;
    else if (const auto* targetTorpedo = world.findTorpedo(action.target.id))
        targetVelocity = targetTorpedo->velocity;
    const auto weaponIndex = static_cast<std::size_t>(player.weapon);
    const double projectileSpeed =
        weaponIndex < world.weapons.size() ? world.weapons[weaponIndex].projectileSpeed : 0.0;
    const Vec3 direction =
        rules.weaponAimDirection(relativePosition, targetVelocity, projectileSpeed);
    player.selectedTarget = action.target.id;
    player.goal.reset();
    if (ship.orbitTransferTargetLevel)
    {
        ship.orbitTransferTargetLevel.reset();
        ship.orbitingPlanet.reset();
        ship.orbitLevel = 0;
    }
    ship.desiredOrientation = orientationToward(direction);
    return {CommandStatus::Applied, "Target selected; ship is aiming at it."};
}

} // namespace spasim::detail
