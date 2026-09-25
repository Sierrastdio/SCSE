#include "spasim/simulation_detail.hpp"

#include <algorithm>
#include <cmath>

namespace spasim::detail {
namespace {

double dot(const Vec3& lhs, const Vec3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
    return {lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x};
}

double approachAngle(double current, double target, double maxDelta) {
    const double delta = std::remainder(target - current, 360.0);
    return std::remainder(current + std::clamp(delta, -maxDelta, maxDelta), 360.0);
}

Vec3 gravityAt(const World& world, const Vec3& position) {
    Vec3 acceleration{};
    for (const auto& planet : world.planets) {
        const Vec3 towardPlanet = planet.position - position;
        const double distance = towardPlanet.length();
        if (distance <= 1e-9 || planet.gravitationalParameter <= 0.0) continue;

        // Newtonian gravity outside the body; uniform-density falloff inside it.
        const double radius = std::max(planet.radius, 1.0);
        const double magnitude = distance < radius
            ? planet.gravitationalParameter * distance / (radius * radius * radius)
            : planet.gravitationalParameter / (distance * distance);
        acceleration += towardPlanet * (magnitude / distance);
    }
    return acceleration;
}

void resolvePlanetSurface(const World& world, Ship& ship) {
    for (const auto& planet : world.planets) {
        const Vec3 fromCenter = ship.position - planet.position;
        const double distance = fromCenter.length();
        const double collisionDistance = planet.radius + ship.collisionRadius;
        if (distance >= collisionDistance) continue;

        const Vec3 normal = distance > 1e-9
            ? fromCenter * (1.0 / distance)
            : Vec3{1.0, 0.0, 0.0};
        ship.position = planet.position + normal * collisionDistance;

        const double inwardSpeed = dot(ship.velocity, normal);
        if (inwardSpeed < 0.0) ship.velocity -= normal * inwardSpeed;
    }
}

void enterParkingOrbit(const Planet& planet, Ship& ship) {
    Vec3 radial = ship.position - planet.position;
    const double distance = radial.length();
    radial = distance > 1e-9 ? radial * (1.0 / distance) : Vec3{1.0, 0.0, 0.0};

    Vec3 tangent = ship.velocity - radial * dot(ship.velocity, radial);
    if (tangent.length() <= 1e-6) tangent = cross({0.0, 0.0, 1.0}, radial);
    if (tangent.length() <= 1e-6) tangent = cross({0.0, 1.0, 0.0}, radial);
    tangent = tangent * (1.0 / tangent.length());

    const double orbitRadius = planet.radius + planet.parkingOrbitAltitude;
    const double orbitSpeed = std::sqrt(planet.gravitationalParameter / orbitRadius);
    ship.position = planet.position + radial * orbitRadius;
    ship.velocity = tangent * orbitSpeed;
    ship.speed = orbitSpeed;
    ship.warp = orbitSpeed;
    ship.acceleration = AccelerationMode::Cruise;
    ship.orbitingPlanet = planet.id;
    ship.orientation = orientationToward(tangent);
    ship.desiredOrientation = ship.orientation;
}

void turnTowardGoal(World& world, const Player* player, Ship& ship, double dt) {
    if (player && player->goal) {
        if (const auto* destination = positionOf(world, *player->goal)) {
            const Vec3 direction = *destination - ship.position;
            if (direction.length() > 1e-9) ship.desiredOrientation = orientationToward(direction);
        }
    }

    const double maxTurn = ship.turnRateDegreesPerSecond * dt;
    ship.orientation.yawDegrees = approachAngle(
        ship.orientation.yawDegrees, ship.desiredOrientation.yawDegrees, maxTurn);
    ship.orientation.pitchDegrees = std::clamp(approachAngle(
        ship.orientation.pitchDegrees, ship.desiredOrientation.pitchDegrees, maxTurn), -90.0, 90.0);
}

Vec3 commandedThrust(const Ship& ship) {
    if (ship.acceleration == AccelerationMode::Accelerate) return forwardVector(ship.orientation);
    if (ship.acceleration == AccelerationMode::Decelerate) {
        const double speed = ship.velocity.length();
        if (speed > 1e-9) return ship.velocity * (-1.0 / speed);
    }
    return {};
}

bool captureGoalOrbit(World& world, Player* player, Ship& ship, const Vec3& previousPosition) {
    if (!player || !player->goal || player->goal->kind != Target::Kind::Planet ||
        player->goal->id >= world.planets.size()) return false;

    const auto& planet = world.planets[player->goal->id];
    const double orbitRadius = planet.radius + planet.parkingOrbitAltitude;
    const double oldDistance = previousPosition.distanceTo(planet.position);
    const double newDistance = ship.position.distanceTo(planet.position);
    const Vec3 radial = newDistance > 1e-9
        ? (ship.position - planet.position) * (1.0 / newDistance)
        : Vec3{1.0, 0.0, 0.0};
    const bool crossedOrbit = oldDistance > orbitRadius && newDistance <= orbitRadius;
    const bool closingInsideOrbit = newDistance <= orbitRadius && dot(ship.velocity, radial) < -1.0;
    if (!crossedOrbit && !closingInsideOrbit) return false;

    enterParkingOrbit(planet, ship);
    player->goal.reset();
    return true;
}

void integrateShip(World& world, Player* player, Ship& ship) {
    const double dt = world.fixedStepSeconds;
    turnTowardGoal(world, player, ship, dt);

    const Vec3 previousPosition = ship.position;
    ship.velocity += (commandedThrust(ship) + gravityAt(world, ship.position)) * dt;
    ship.position += ship.velocity * dt;

    captureGoalOrbit(world, player, ship, previousPosition);
    resolvePlanetSurface(world, ship);

    ship.speed = ship.velocity.length();
    ship.warp = ship.speed; // Retained for the existing console and SPASIM-facing API.
    if (ship.acceleration != AccelerationMode::Cruise)
        ship.resources.energy = std::max(0.0, ship.resources.energy - dt);
}

} // namespace

Vec3 gravityAcceleration(const World& world, const Vec3& position) {
    return gravityAt(world, position);
}

void updateMovement(World& world) {
    for (auto& slot : world.ships) {
        if (!slot || !slot->playerControlled) continue;
        auto& ship = *slot;
        if (!ship.exists || ship.combat.destroyed || ship.resources.energy <= 0.0) continue;
        integrateShip(world, world.findPlayer(ship.owner), ship);
    }
}

} // namespace spasim::detail
