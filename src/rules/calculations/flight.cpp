#include "rules/game_rules.hpp"

#include <algorithm>
#include <cmath>

namespace spasim
{

double GameRules::speedLimit(std::uint8_t throttleLevel, double gravitationalParameter,
  double sphereOfInfluenceRadius) const noexcept
{
  if (throttleLevel == 0 || throttleLevel > throttleSpeedMultipliers.size() || gravitationalParameter <= 0.0 ||
      sphereOfInfluenceRadius <= 0.0)
  {
    return 0.0;
  }

  const double referenceSpeed = std::sqrt(gravitationalParameter / sphereOfInfluenceRadius);
  return referenceSpeed * throttleSpeedMultipliers[throttleLevel - 1];
}

AccelerationMode GameRules::approachThrottle(double distanceToCapture, double currentSpeed, double closingSpeed,
  double maximumSpeed, double deltaSeconds) const
{
  const double remaining = std::max(0.0, distanceToCapture - currentSpeed * deltaSeconds);
  const double safeSpeed = std::sqrt(2.0 * brakingAcceleration * remaining);
  const double targetSpeed = std::min(maximumSpeed, safeSpeed);
  const double tolerance = std::max(0.05, std::max(thrustAcceleration, brakingAcceleration) * deltaSeconds);

  if (currentSpeed > targetSpeed + tolerance)
  {
    return AccelerationMode::Decelerate;
  }
  if (closingSpeed < targetSpeed - tolerance)
  {
    return AccelerationMode::Accelerate;
  }
  return AccelerationMode::Cruise;
}

} // namespace spasim
