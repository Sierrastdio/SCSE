#include "engine/detail.hpp"

#include <algorithm>
#include <cmath>

namespace spasim::detail
{
namespace
{

double dot(const Vec3& lhs, const Vec3& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 cross(const Vec3& lhs, const Vec3& rhs)
{
    return {lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x};
}

double approachAngle(double current, double target, double maxDelta)
{
    const double delta = std::remainder(target - current, 360.0);
    return std::remainder(current + std::clamp(delta, -maxDelta, maxDelta), 360.0);
}

Vec3 gravityAt(const World& world, const Vec3& position)
{
    Vec3 acceleration{};
    for (const auto& planet : world.planets)
    {
        const Vec3 towardPlanet = planet.position - position;
        const double distanceSquared = towardPlanet.lengthSquared();
        if (distanceSquared <= 1e-18 || planet.gravitationalParameter <= 0.0)
            continue;

        // Newtonian gravity outside the body; uniform-density falloff inside it.
        const double radius = std::max(planet.radius, 1.0);
        const double inverseDistance = 1.0 / std::sqrt(distanceSquared);
        const double scale =
            distanceSquared < radius * radius
                ? planet.gravitationalParameter / (radius * radius * radius)
                : planet.gravitationalParameter * inverseDistance / distanceSquared;
        acceleration += towardPlanet * scale;
    }
    return acceleration;
}

void resolvePlanetSurface(const World& world, Ship& ship)
{
    for (const auto& planet : world.planets)
    {
        const Vec3 fromCenter = ship.position - planet.position;
        const double collisionDistance = planet.radius + ship.collisionRadius;
        const double distanceSquared = fromCenter.lengthSquared();
        if (distanceSquared >= collisionDistance * collisionDistance)
            continue;

        const double distance = std::sqrt(distanceSquared);
        const Vec3 normal = distance > 1e-9 ? fromCenter * (1.0 / distance) : Vec3{1.0, 0.0, 0.0};
        ship.position = planet.position + normal * collisionDistance;

        const double inwardSpeed = dot(ship.velocity, normal);
        if (inwardSpeed < 0.0)
            ship.velocity -= normal * inwardSpeed;
    }
}

void turnTowardGoal(
    World& world, const Player* player, Ship& ship, double dt, const GameRules& rules);

bool integrateStableOrbit(
    World& world, Player* player, Ship& ship, double dt, const GameRules& rules)
{
    if (!ship.orbitingPlanet || ship.orbitTransferTargetLevel || ship.orbitLevel == 0 ||
        *ship.orbitingPlanet >= world.planets.size() || dt <= 0.0)
        return false;

    const Planet& planet = world.planets[*ship.orbitingPlanet];
    if (ship.orbitLevel > planet.orbitLevelCount || planet.gravitationalParameter <= 0.0)
        return false;

    const double orbitRadius = planet.orbitLevelRadii[ship.orbitLevel - 1];
    const Vec3 relativePosition = ship.position - planet.position;
    const double radius = relativePosition.length();
    if (radius <= 1e-9 || orbitRadius <= 0.0)
        return false;

    const Vec3 radial = relativePosition * (1.0 / radius);
    Vec3 tangentVelocity = ship.velocity - radial * dot(ship.velocity, radial);
    if (tangentVelocity.length() <= 1e-6)
        tangentVelocity = cross({0.0, 0.0, 1.0}, radial);
    if (tangentVelocity.length() <= 1e-6)
        tangentVelocity = cross({0.0, 1.0, 0.0}, radial);
    const Vec3 tangent = tangentVelocity * (1.0 / tangentVelocity.length());

    const double angularRate =
        std::sqrt(planet.gravitationalParameter / (orbitRadius * orbitRadius * orbitRadius));
    const double angle = angularRate * dt;
    const Vec3 nextRadial = radial * std::cos(angle) + tangent * std::sin(angle);
    const Vec3 nextTangent = tangent * std::cos(angle) - radial * std::sin(angle);
    const double orbitSpeed = std::sqrt(planet.gravitationalParameter / orbitRadius);

    ship.position = planet.position + nextRadial * orbitRadius;
    ship.velocity = nextTangent * orbitSpeed;
    ship.speed = orbitSpeed;
    ship.warp = orbitSpeed;
    ship.acceleration = AccelerationMode::Cruise;
    if (player && player->selectedTarget)
        turnTowardGoal(world, player, ship, dt, rules);
    else
    {
        ship.orientation = orientationToward(nextTangent);
        ship.desiredOrientation = ship.orientation;
    }
    return true;
}

bool integrateOrbitTransfer(World& world, Ship& ship, double dt, const GameRules& rules)
{
    if (!ship.orbitingPlanet || !ship.orbitTransferTargetLevel)
        return false;

    const Planet& planet = world.planets[*ship.orbitingPlanet];

    const std::uint8_t targetLevel = *ship.orbitTransferTargetLevel;

    const double targetRadius = planet.orbitLevelRadii[targetLevel - 1];

    const Vec3 relativePosition = ship.position - planet.position;

    const double radius = relativePosition.length();

    if (radius <= 1e-9 || dt <= 0.0)
        return false;

    const Vec3 radial = relativePosition * (1.0 / radius);
    Vec3 tangentVelocity = ship.velocity - radial * dot(ship.velocity, radial);
    if (tangentVelocity.length() <= 1e-6)
        tangentVelocity = cross({0.0, 0.0, 1.0}, radial);
    if (tangentVelocity.length() <= 1e-6)
        tangentVelocity = cross({0.0, 1.0, 0.0}, radial);
    const Vec3 tangent = tangentVelocity * (1.0 / tangentVelocity.length());
    Vec3 normal = cross(radial, tangent);
    const double normalLength = normal.length();
    if (normalLength <= 1e-9)
        return false;

    const double requestedStep = rules.transferRadialStep(radius, targetRadius, dt);

    const double radialStep = std::clamp(
        requestedStep, -std::abs(targetRadius - radius), std::abs(targetRadius - radius));

    const double nextRadius = radius + radialStep;

    const double gravitationalParameter = std::max(planet.gravitationalParameter, 0.0);

    const double orbitalRate = std::sqrt(gravitationalParameter / (radius * radius * radius));

    const double angle = orbitalRate * dt;

    const Vec3 nextRadial = radial * std::cos(angle) + tangent * std::sin(angle);

    const Vec3 nextTangent = tangent * std::cos(angle) - radial * std::sin(angle);

    ship.position = planet.position + nextRadial * nextRadius;
    const double targetSpeed = std::sqrt(gravitationalParameter / nextRadius);
    const double radialSpeed = radialStep / dt;
    ship.velocity = nextTangent * targetSpeed + nextRadial * radialSpeed;
    ship.acceleration = AccelerationMode::Cruise;

    if (std::abs(targetRadius - nextRadius) <= 0.01)
    {
        ship.position = planet.position + nextRadial * targetRadius;
        ship.velocity = nextTangent * targetSpeed;
        ship.orbitLevel = targetLevel;
        ship.orbitTransferTargetLevel.reset();
    }

    ship.speed = ship.velocity.length();
    ship.warp = ship.speed;
    ship.orientation = orientationToward(nextTangent);
    ship.desiredOrientation = ship.orientation;
    return true;
}

void turnTowardGoal(
    World& world, const Player* player, Ship& ship, double dt, const GameRules& rules)
{
    if (player && player->goal)
    {
        if (const auto* destination = positionOf(world, *player->goal))
        {
            const Vec3 direction = *destination - ship.position;
            if (direction.length() > 1e-9)
                ship.desiredOrientation = orientationToward(direction);
        }
    }
    else if (player && player->selectedTarget)
    {
        const Target target{Target::Kind::Entity, *player->selectedTarget};
        if (const Vec3* targetPosition = positionOf(world, target))
        {
            const Vec3 relativePosition = *targetPosition - ship.position;
            if (relativePosition.length() <= 1e-9)
                return;
            Vec3 targetVelocity{};
            if (const auto* targetShip = world.findShip(*player->selectedTarget))
                targetVelocity = targetShip->velocity;
            else if (const auto* targetTorpedo = world.findTorpedo(*player->selectedTarget))
                targetVelocity = targetTorpedo->velocity;
            const auto weaponIndex = static_cast<std::size_t>(player->weapon);
            const Vec3 aimDirection = rules.weaponAimDirection(
                relativePosition,
                targetVelocity,
                weaponIndex < world.weapons.size() ? world.weapons[weaponIndex].projectileSpeed
                                                   : 0.0);
            ship.desiredOrientation = orientationToward(aimDirection);
        }
    }

    const double maxTurn = ship.turnRateDegreesPerSecond * dt;
    ship.orientation.yawDegrees =
        approachAngle(ship.orientation.yawDegrees, ship.desiredOrientation.yawDegrees, maxTurn);
    ship.orientation.pitchDegrees = std::clamp(
        approachAngle(ship.orientation.pitchDegrees, ship.desiredOrientation.pitchDegrees, maxTurn),
        -90.0,
        90.0);
}

void updateApproachThrottle(
    World& world, const Player* player, Ship& ship, double dt, const GameRules& rules)
{
    if (!player || !player->goal || player->goal->kind != Target::Kind::Planet ||
        player->goal->id >= world.planets.size())
        return;

    const Planet& target = world.planets[player->goal->id];
    const double distanceToInfluence =
        std::max(0.0, ship.position.distanceTo(target.position) - target.sphereOfInfluenceRadius);
    const double maximumSpeed = rules.speedLimit(
        ship.throttleLevel, target.gravitationalParameter, target.sphereOfInfluenceRadius);
    const Vec3 towardTarget = target.position - ship.position;
    const double distance = towardTarget.length();
    const double closingSpeed =
        distance > 1e-9 ? dot(ship.velocity, towardTarget * (1.0 / distance)) : 0.0;
    ship.acceleration = rules.approachThrottle(
        distanceToInfluence, ship.velocity.length(), closingSpeed, maximumSpeed, dt);
}

Vec3 commandedThrust(const Ship& ship, const GameRules& rules, double maximumSpeed)
{
    if (ship.acceleration == AccelerationMode::Accelerate)
    {
        Vec3 thrust = forwardVector(ship.orientation) * rules.thrustAcceleration;
        const double speed = ship.velocity.length();
        if (speed >= maximumSpeed && speed > 1e-9)
        {
            const Vec3 velocityDirection = ship.velocity * (1.0 / speed);
            const double forwardAcceleration = dot(thrust, velocityDirection);
            if (forwardAcceleration > 0.0)
                thrust -= velocityDirection * forwardAcceleration;
        }
        return thrust;
    }
    if (ship.acceleration == AccelerationMode::Decelerate)
    {
        const double speed = ship.velocity.length();
        if (speed > 1e-9)
            return ship.velocity * (-rules.brakingAcceleration / speed);
    }
    return {};
}

bool captureGoalOrbit(World& world, Player* player, Ship& ship, const Vec3& startPosition)
{
    if (!player || !player->goal || player->goal->kind != Target::Kind::Planet ||
        player->goal->id >= world.planets.size())
        return false;

    const auto& planet = world.planets[player->goal->id];
    const double orbitRadius = planet.sphereOfInfluenceRadius;
    const Vec3 startOffset = startPosition - planet.position;
    const double startDistanceSquared = dot(startOffset, startOffset);
    const double influenceRadiusSquared = orbitRadius * orbitRadius;

    // If the ship starts inside the influence sphere, its current position is
    // already the capture point. Otherwise sweep the whole tick segment so a
    // fast ship cannot skip across the sphere between fixed-step samples.
    if (startDistanceSquared > influenceRadiusSquared)
    {
        const Vec3 segment = ship.position - startPosition;
        const double segmentLengthSquared = dot(segment, segment);
        if (segmentLengthSquared <= 1e-12)
            return false;

        const double closestFraction =
            std::clamp(-dot(startOffset, segment) / segmentLengthSquared, 0.0, 1.0);
        const Vec3 closestOffset = startOffset + segment * closestFraction;
        const double closestDistanceSquared = dot(closestOffset, closestOffset);
        if (closestDistanceSquared > influenceRadiusSquared)
            return false;

        const double halfChordFraction =
            std::sqrt((influenceRadiusSquared - closestDistanceSquared) / segmentLengthSquared);
        const double entryFraction = closestFraction - halfChordFraction;
        if (entryFraction < 0.0 || entryFraction > 1.0)
            return false;

        // Stop at the actual influence-boundary crossing for this tick. This
        // is a swept collision/capture event, not a reset to an orbit radius.
        ship.position = startPosition + segment * entryFraction;
    }
    const auto requestedLevel = player->goalOrbitLevel;
    // Capture starts from the actual crossing position and velocity. Do not
    // reset the ship to level 1; the Lua transfer policy handles the remaining
    // radial distance to the requested level from here.
    ship.orbitingPlanet = planet.id;
    ship.orbitLevel = 0;
    ship.orbitTransferTargetLevel = requestedLevel;
    ship.acceleration = AccelerationMode::Cruise;
    player->goal.reset();
    player->goalOrbitLevel = requestedLevel;
    return true;
}

bool completeOrbitTransfer(const World& world, Ship& ship)
{
    if (!ship.orbitingPlanet || !ship.orbitTransferTargetLevel)
        return false;
    const auto& planet = world.planets[*ship.orbitingPlanet];
    const auto level = *ship.orbitTransferTargetLevel;
    const double targetRadius = planet.orbitLevelRadii[level - 1];
    const Vec3 relativePosition = ship.position - planet.position;
    const double radius = relativePosition.length();
    if (radius <= 1e-9)
        return false;
    const Vec3 radial = relativePosition * (1.0 / radius);
    const double radialSpeed = dot(ship.velocity, radial);
    Vec3 tangent = ship.velocity - radial * radialSpeed;
    const double tangentialSpeed = tangent.length();
    const double targetSpeed = std::sqrt(planet.gravitationalParameter / targetRadius);
    if (std::abs(targetRadius - radius) > 2.0 || std::abs(radialSpeed) > 0.5 ||
        std::abs(tangentialSpeed - targetSpeed) > 0.5)
        return false;
    if (tangentialSpeed <= 1e-6)
        tangent = cross({0.0, 0.0, 1.0}, radial);
    if (tangent.length() <= 1e-6)
        tangent = cross({0.0, 1.0, 0.0}, radial);
    tangent = tangent * (1.0 / tangent.length());
    ship.position = planet.position + radial * targetRadius;
    ship.velocity = tangent * targetSpeed;
    ship.speed = targetSpeed;
    ship.warp = targetSpeed;
    ship.orbitLevel = level;
    ship.orbitTransferTargetLevel.reset();
    ship.acceleration = AccelerationMode::Cruise;
    ship.orientation = orientationToward(tangent);
    ship.desiredOrientation = ship.orientation;
    return true;
}

void integrateShip(World& world, Player* player, Ship& ship, const GameRules& rules)
{
    const double dt = world.fixedStepSeconds;
    if (integrateOrbitTransfer(world, ship, dt, rules))
        return;
    if (integrateStableOrbit(world, player, ship, dt, rules))
        return;

    if (captureGoalOrbit(world, player, ship, ship.position))
    {
        integrateOrbitTransfer(world, ship, dt, rules);
        return;
    }

    turnTowardGoal(world, player, ship, dt, rules);
    updateApproachThrottle(world, player, ship, dt, rules);

    const Vec3 previousPosition = ship.position;
    double speedLimit = rules.speedLimit(
        ship.throttleLevel, rules.referenceGravitationalParameter, rules.sphereOfInfluenceRadius);
    if (player && player->goal && player->goal->kind == Target::Kind::Planet &&
        player->goal->id < world.planets.size())
    {
        const Planet& target = world.planets[player->goal->id];
        speedLimit = rules.speedLimit(
            ship.throttleLevel, target.gravitationalParameter, target.sphereOfInfluenceRadius);
    }
    const double speedBeforeStep = ship.velocity.length();
    ship.velocity +=
        (commandedThrust(ship, rules, speedLimit) + gravityAt(world, ship.position)) * dt;
    if (ship.acceleration == AccelerationMode::Accelerate)
    {
        const double speed = ship.velocity.length();
        if (speedBeforeStep <= speedLimit && speed > speedLimit && speed > 1e-9)
            ship.velocity = ship.velocity * (speedLimit / speed);
    }
    ship.position += ship.velocity * dt;

    captureGoalOrbit(world, player, ship, previousPosition);
    completeOrbitTransfer(world, ship);
    resolvePlanetSurface(world, ship);

    ship.speed = ship.velocity.length();
    ship.warp = ship.speed; // Kept for compatibility with existing state consumers.
    if (ship.acceleration != AccelerationMode::Cruise)
        ship.resources.energy = std::max(0.0, ship.resources.energy - dt);
}

} // namespace

Vec3 gravityAcceleration(const World& world, const Vec3& position)
{
    return gravityAt(world, position);
}

void updateMovement(World& world, const GameRules& rules)
{
    for (auto& slot : world.ships)
    {
        if (!slot || !slot->playerControlled)
            continue;
        auto& ship = *slot;
        if (!ship.exists || ship.combat.destroyed || ship.resources.energy <= 0.0)
            continue;
        integrateShip(world, world.findPlayer(ship.owner), ship, rules);
    }
}

} // namespace spasim::detail
