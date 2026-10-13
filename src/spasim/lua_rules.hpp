#pragma once

#include "spasim/model.hpp"

#include <array>
#include <filesystem>
#include <memory>

namespace spasim
{

// Validated scenario data and live policies loaded from Lua.
class GameRules
{
  public:
    explicit GameRules(const std::filesystem::path& scriptPath);
    ~GameRules();

    GameRules(GameRules&&) noexcept;
    GameRules& operator=(GameRules&&) noexcept;
    GameRules(const GameRules&) = delete;
    GameRules& operator=(const GameRules&) = delete;

    double sphereOfInfluenceRadius{};
    std::uint8_t levelCount{};
    std::array<std::uint8_t, 3> maximumLevelByClass{};
    std::array<double, kMaxOrbitLevels> radiusFractions{};
    std::array<double, 4> throttleSpeedMultipliers{};
    double referenceGravitationalParameter{};
    double thrustAcceleration{};
    double brakingAcceleration{};
    double maximumAimLeadSeconds{};
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
    WeaponKind defaultWeapon{};
    std::array<Team, kTeamCount> teams{};
    std::vector<Planet> planets;
    std::array<WeaponProfile, kWeaponCount> weapons{};
    std::array<std::string, kPracticeShipCount> practiceNames{};
    std::array<Vec3, kPracticeShipCount> practicePositions{};

    void applyScenario(World& world) const;
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
    [[nodiscard]] double
    transferRadialStep(double currentRadius, double targetRadius, double deltaSeconds) const;

  private:
    struct Runtime;
    std::unique_ptr<Runtime> runtime_;
};

} // namespace spasim
