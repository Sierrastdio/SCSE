#include "rules/game_rules.hpp"

#include <algorithm>
#include <cmath>

namespace spasim
{

std::uint8_t GameRules::maximumLevel(ShipClass shipClass) const noexcept
{
  const auto index = static_cast<std::size_t>(shipClass);
  return index < maximumLevelByClass.size() ? maximumLevelByClass[index] : 1;
}

double GameRules::orbitRadius(const Planet &planet, std::uint8_t level) const noexcept
{
  if (level == 0 || level > levelCount)
  {
    return 0.0;
  }

  const double innerRadius = planet.radius + planet.parkingOrbitAltitude;

  return innerRadius + (planet.sphereOfInfluenceRadius - innerRadius) * radiusFractions[level - 1];
}

double GameRules::transferRadialStep(double currentRadius, double targetRadius, double deltaSeconds) const
{
  const double difference = targetRadius - currentRadius;

  const double remaining = std::abs(difference);

  if (remaining <= 0.01)
  {
    return difference;
  }

  const double speed = std::min(transferMaximumRadialSpeed, std::sqrt(2.0 * transferRadialAcceleration * remaining));

  return (difference > 0.0 ? 1.0 : -1.0) * std::min(remaining, speed * deltaSeconds);
}

} // namespace spasim
