#include "spasim/simulation_detail.hpp"

#include <cmath>
#include <numbers>

namespace spasim::detail
{

Vec3 forwardVector(const Orientation& orientation)
{
    constexpr double toRadians = std::numbers::pi / 180.0;
    const double yaw = orientation.yawDegrees * toRadians;
    const double pitch = orientation.pitchDegrees * toRadians;
    return {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
}

Orientation orientationToward(const Vec3& direction)
{
    return {std::atan2(direction.y, direction.x) * 180.0 / std::numbers::pi,
            std::atan2(direction.z, std::hypot(direction.x, direction.y)) * 180.0 /
                std::numbers::pi};
}

CombatState* combatState(World& world, Target target)
{
    if (target.kind != Target::Kind::Entity)
        return nullptr;
    if (auto* ship = world.findShip(target.id))
        return &ship->combat;
    if (auto* base = world.findBase(target.id))
        return &base->combat;
    return nullptr;
}

std::optional<TeamId> teamOf(const World& world, EntityId id)
{
    if (const auto* ship = world.findShip(id))
        return ship->team;
    if (const auto* base = world.findBase(id))
        return base->team;
    if (const auto* torpedo = world.findTorpedo(id))
        return torpedo->team;
    return std::nullopt;
}

Vec3* positionOf(World& world, Target target)
{
    if (target.kind == Target::Kind::Planet)
        return target.id < world.planets.size() ? &world.planets[target.id].position : nullptr;
    if (auto* ship = world.findShip(target.id))
        return &ship->position;
    if (auto* base = world.findBase(target.id))
        return &base->position;
    if (auto* torpedo = world.findTorpedo(target.id))
        return &torpedo->position;
    return nullptr;
}

} // namespace spasim::detail
