#include "rules/game_rules.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace spasim
{
namespace
{

double dot(const Vec3 &lhs, const Vec3 &rhs) noexcept { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }

} // namespace

Vec3 GameRules::weaponAimDirection(const Vec3 &relativePosition, const Vec3 &relativeVelocity, double projectileSpeed,
  const WeaponAimSettings &aim) const
{
  const double distanceSquared = relativePosition.lengthSquared();

  if (distanceSquared <= 1.0e-12 || projectileSpeed <= 1.0e-9)
  {
    const double length = relativePosition.length();

    if (!std::isfinite(length) || length <= 1.0e-9)
    {
      throw std::runtime_error("Weapon aim direction must be finite and non-zero.");
    }
    return relativePosition * (1.0 / length);
  }

  const double velocitySquared = relativeVelocity.lengthSquared();
  const double a = velocitySquared - projectileSpeed * projectileSpeed;
  const double b = 2.0 * dot(relativePosition, relativeVelocity);
  double time = 0.0;

  if (std::abs(a) <= 1.0e-9)
  {
    if (b < -1.0e-9)
    {
      time = -distanceSquared / b;
    }
  }
  else
  {
    const double discriminant = b * b - 4.0 * a * distanceSquared;

    if (discriminant >= 0.0)
    {
      const double root = std::sqrt(discriminant);
      const double firstTime = (-b - root) / (2.0 * a);
      const double secondTime = (-b + root) / (2.0 * a);

      if (firstTime > 0.0 && secondTime > 0.0)
      {
        time = std::min(firstTime, secondTime);
      }
      else
      {
        time = std::max({firstTime, secondTime, 0.0});
      }
    }
  }

  time = std::clamp(time, 0.0, aim.maximumLeadSeconds);
  const Vec3 direction = relativePosition + relativeVelocity * time;
  const double length = direction.length();

  if (!std::isfinite(length) || length <= 1.0e-9)
  {
    throw std::runtime_error("Weapon aim direction must be finite and non-zero.");
  }
  return direction * (1.0 / length);
}

bool GameRules::isWithinFiringArc(const Orientation &orientation, const Vec3 &targetDirection,
  const WeaponAimSettings &aim) const noexcept
{
  const double horizontalLength = std::hypot(targetDirection.x, targetDirection.y);
  if (!std::isfinite(horizontalLength) || !std::isfinite(targetDirection.z) || targetDirection.lengthSquared() <= 1e-18)
  {
    return false;
  }

  constexpr double toDegrees = 180.0 / std::numbers::pi;
  const double targetYaw = std::atan2(targetDirection.y, targetDirection.x) * toDegrees;
  const double targetPitch = std::atan2(targetDirection.z, horizontalLength) * toDegrees;
  const double horizontalError = horizontalLength <= 1e-9
    ? 0.0
    : std::abs(std::remainder(targetYaw - orientation.yawDegrees, 360.0));
  const double verticalError = std::abs(targetPitch - orientation.pitchDegrees);

  return horizontalError <= aim.maximumHorizontalFiringAngleDegrees
      && verticalError <= aim.maximumVerticalFiringAngleDegrees;
}

} // namespace spasim
