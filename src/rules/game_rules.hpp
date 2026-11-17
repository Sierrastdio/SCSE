#pragma once

#include "engine/types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace spasim
{

struct PracticeShipDefinition
{
  TeamId team{};
  std::string name;
  Vec3 position{};
};

class GameRules
{
public:
  GameRules();

  double sphereOfInfluenceRadius{};
  double transferMaximumRadialSpeed{};
  double transferRadialAcceleration{};

  std::uint8_t levelCount{};
  std::array<std::uint8_t, 3> maximumLevelByClass{};
  std::array<double, kMaxOrbitLevels> radiusFractions{};
  std::array<double, 4> throttleSpeedMultipliers{};

  double referenceGravitationalParameter{};
  double thrustAcceleration{};
  double brakingAcceleration{};

  std::uint64_t randomSeed{};
  double fixedStepSeconds{};
  double sessionDurationSeconds{};
  bool friendlyFire{};
  bool paused{};

  Player playerDefaults{};
  Ship playerShipDefaults{};
  Ship practiceShipDefaults{};
  Base baseDefaults{};

  double torpedoLaunchSpeed{};
  double torpedoLifeSeconds{};
  double torpedoCollisionRadius{};

  EntityId firstEntityId{1};
  PlayerId firstPlayerId{};
  WeaponId defaultWeapon{};
  std::size_t playersPerTeam{};
  std::size_t shipsPerPlayer{};

  std::vector<Team> teams;
  std::vector<Planet> planets;
  std::vector<WeaponProfile> weapons;
  std::vector<PracticeShipDefinition> practiceShips;

  void applyScenario(World &world) const;

  [[nodiscard]] std::uint8_t maximumLevel(ShipClass shipClass) const noexcept;
  [[nodiscard]] double orbitRadius(const Planet &planet, std::uint8_t level) const noexcept;
  [[nodiscard]] double speedLimit(std::uint8_t throttleLevel, double gravitationalParameter,
    double sphereOfInfluenceRadius) const noexcept;
  [[nodiscard]] AccelerationMode approachThrottle(double distanceToCapture, double currentSpeed, double closingSpeed,
    double maximumSpeed, double deltaSeconds) const;
  [[nodiscard]] Vec3 weaponAimDirection(const Vec3 &relativePosition, const Vec3 &relativeVelocity, double projectileSpeed,
    const WeaponAimSettings &aim) const;
  [[nodiscard]] bool isWithinFiringArc(const Orientation &orientation, const Vec3 &targetDirection,
    const WeaponAimSettings &aim) const noexcept;
  [[nodiscard]] double transferRadialStep(double currentRadius, double targetRadius, double deltaSeconds) const;
};

} // namespace spasim
