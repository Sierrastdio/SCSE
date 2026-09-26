#pragma once

#include "spasim/model.hpp"

#include <array>
#include <filesystem>
#include <memory>

namespace spasim
{

// Validated Lua game rules and the live runtime policies they define.
class OrbitRules
{
  public:
    explicit OrbitRules(const std::filesystem::path& scriptPath);
    ~OrbitRules();

    OrbitRules(OrbitRules&&) noexcept;
    OrbitRules& operator=(OrbitRules&&) noexcept;
    OrbitRules(const OrbitRules&) = delete;
    OrbitRules& operator=(const OrbitRules&) = delete;

    double sphereOfInfluenceRadius{};
    std::uint8_t levelCount{};
    std::array<std::uint8_t, 3> maximumLevelByClass{};
    std::array<double, kMaxOrbitLevels> radiusFractions{};
    std::array<double, 4> throttleSpeedMultipliers{};
    double referenceGravitationalParameter{};
    double thrustAcceleration{};
    double brakingAcceleration{};
    double maximumAimLeadSeconds{};

    [[nodiscard]] std::uint8_t maximumLevel(ShipClass shipClass) const noexcept;
    [[nodiscard]] double orbitRadius(const Planet& planet, std::uint8_t level) const noexcept;
    [[nodiscard]] double speedLimit(std::uint8_t throttleLevel,
                                    double gravitationalParameter,
                                    double sphereOfInfluenceRadius) const noexcept;
    [[nodiscard]] AccelerationMode approachThrottle(double distanceToCapture,
                                                    double currentSpeed,
                                                    double closingSpeed,
                                                    double maximumSpeed,
                                                    double deltaSeconds) const;
    [[nodiscard]] Vec3 weaponAimDirection(const Vec3& relativePosition,
                                          const Vec3& relativeVelocity,
                                          double projectileSpeed) const;
    [[nodiscard]] double transferRadialStep(double currentRadius,
                                            double targetRadius,
                                            double deltaSeconds) const;

  private:
    struct Runtime;
    std::unique_ptr<Runtime> runtime_;
};

} // namespace spasim
