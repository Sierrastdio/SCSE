#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace spasim
{

inline constexpr std::size_t kTeamCount = 4;
inline constexpr std::size_t kPlayersPerTeam = 6;
inline constexpr std::size_t kPlayerCapacity = kTeamCount * kPlayersPerTeam;
// Leave entity storage room for static training targets as well as every player.
inline constexpr std::size_t kPracticeShipCount = kTeamCount;
inline constexpr std::size_t kEntityCapacity = kPlayerCapacity + kPracticeShipCount;
inline constexpr std::size_t kWeaponCount = 4;
inline constexpr std::size_t kMaxOrbitLevels = 8;

using PlayerId = std::uint32_t;
using EntityId = std::uint32_t;
using PlanetId = std::uint8_t;
using TeamId = std::uint8_t;
inline constexpr PlayerId kNoPlayer = UINT32_MAX;

struct Vec3
{
    double x{};
    double y{};
    double z{};

    [[nodiscard]] double lengthSquared() const noexcept;
    [[nodiscard]] double length() const noexcept;
    [[nodiscard]] double distanceTo(const Vec3& other) const noexcept;
    Vec3& operator+=(const Vec3& other) noexcept;
    Vec3& operator-=(const Vec3& other) noexcept;
};

[[nodiscard]] Vec3 operator+(Vec3 lhs, const Vec3& rhs) noexcept;
[[nodiscard]] Vec3 operator-(Vec3 lhs, const Vec3& rhs) noexcept;
[[nodiscard]] Vec3 operator*(Vec3 value, double scalar) noexcept;

struct Orientation
{
    double yawDegrees{};
    double pitchDegrees{};
};

enum class ObjectKind : std::uint8_t
{
    Ship,
    Base,
    Torpedo
};
enum class WeaponKind : std::uint8_t
{
    Slot0,
    Slot1,
    Slot2,
    Slot3
};
enum class AccelerationMode : std::uint8_t
{
    Decelerate,
    Cruise,
    Accelerate
};
enum class ShipClass : std::uint8_t
{
    Small,
    Medium,
    Large
};

struct Resources
{
    double energy{};
    double rawMaterial{};
};

struct CombatState
{
    double hull{};
    double hullDamage{};
    double shields{};
    double skill{};
    bool destroyed{};
};

struct WeaponProfile
{
    std::string name;
    double energyCost{};
    double damage{};
    double range{};
    double projectileSpeed{};
    bool explosive{};
    double splashRadius{};
};

struct Ship
{
    EntityId id{};
    PlayerId owner{kNoPlayer};
    TeamId team{};
    std::string name;
    ShipClass shipClass{};
    Vec3 position{};
    Vec3 velocity{}; // Inertial world-units per second; heading and velocity are independent.
    Orientation orientation{};
    Orientation desiredOrientation{};
    double turnRateDegreesPerSecond{};
    double speed{};
    double warp{};
    AccelerationMode acceleration{};
    std::uint8_t throttleLevel{};
    Resources resources{};
    CombatState combat{};
    double collisionRadius{};
    std::optional<PlanetId> orbitingPlanet;
    std::uint8_t orbitLevel{};
    std::optional<std::uint8_t> orbitTransferTargetLevel;
    bool playerControlled{};
    bool exists{};
};

struct Base
{
    EntityId id{};
    PlayerId owner{};
    TeamId team{};
    PlanetId homePlanet{};
    Vec3 position{};
    Resources resources{};
    CombatState combat{};
    double collisionRadius{};
    bool exists{};
};

struct Torpedo
{
    EntityId id{};
    PlayerId owner{};
    TeamId team{};
    Vec3 position{};
    Vec3 velocity{};
    double energy{};
    double warp{};
    double fieldStrength{};
    double damage{};
    double lifeSeconds{};
    double collisionRadius{};
    bool controlled{};
    bool detonated{};
    bool exists{};
};

struct Target
{
    enum class Kind : std::uint8_t
    {
        Entity,
        Planet
    };
    Kind kind{Kind::Entity};
    std::uint32_t id{};
};

struct Player
{
    PlayerId id{};
    TeamId team{};
    std::string callSign;
    std::string station;
    EntityId shipId{};
    EntityId baseId{};
    EntityId torpedoId{};
    EntityId selectedObject{};
    std::optional<Target> goal;
    std::uint8_t goalOrbitLevel{};
    double phasorPower{};
    double phasorDiameter{};
    WeaponKind weapon{WeaponKind::Slot0};
    std::optional<EntityId> selectedTarget;
    bool phasorEnabled{};
    bool joined{};
};

struct Planet
{
    PlanetId id{};
    std::string name;
    Vec3 position{};
    double radius{};
    double gravitationalParameter{}; // G * mass in the simulation's world units.
    double parkingOrbitAltitude{};
    double sphereOfInfluenceRadius{};
    std::uint8_t orbitLevelCount{};
    std::array<double, kMaxOrbitLevels> orbitLevelRadii{};
    double population{};
    double antiEntropy{};
    double rawMaterial{};
    double standardOfLiving{};
    double skill{};
    std::optional<TeamId> controllingTeam;
    double revoltAtSeconds{};
};

struct Team
{
    TeamId id{};
    PlanetId homePlanet{};
    ShipClass shipClass{};
    std::string name;
    Vec3 spawnOffset{};
    Vec3 baseOffset{};
    double spawnRadius{};
    double spawnVerticalMinimum{};
    double spawnVerticalMaximum{};
    std::uint8_t startingOrbitLevel{};
    bool randomSpawn{};
    std::array<bool, kTeamCount> allied{};
};

struct World
{
    std::array<Team, kTeamCount> teams{};
    std::vector<Planet> planets;
    std::array<std::optional<Player>, kPlayerCapacity> playerSlots{};
    // Fixed-capacity slot stores keep references stable and make iteration predictable.
    std::array<std::optional<Ship>, kEntityCapacity> ships{};
    std::array<std::optional<Base>, kEntityCapacity> bases{};
    std::array<std::optional<Torpedo>, kEntityCapacity> torpedoes{};
    std::uint64_t tick{};
    double elapsedSeconds{};
    double fixedStepSeconds{};
    std::mt19937_64 random;
    std::optional<std::uint64_t> randomSeedOverride;
    EntityId nextEntityId{};
    EntityId entityIdStart{};
    PlayerId playerIdStart{};
    WeaponKind defaultWeapon{};
    bool friendlyFire{};
    bool paused{};
    double sessionDurationSeconds{};
    std::array<WeaponProfile, kWeaponCount> weapons{};
    Player playerDefaults{};
    Ship playerShipDefaults{};
    Ship practiceShipDefaults{};
    Base baseDefaults{};
    double torpedoLaunchSpeed{};
    double torpedoLifeSeconds{};
    double torpedoCollisionRadius{};

    [[nodiscard]] const Player* findPlayer(PlayerId id) const noexcept;
    [[nodiscard]] Player* findPlayer(PlayerId id) noexcept;
    [[nodiscard]] Ship* findShip(EntityId id) noexcept;
    [[nodiscard]] const Ship* findShip(EntityId id) const noexcept;
    [[nodiscard]] Base* findBase(EntityId id) noexcept;
    [[nodiscard]] Torpedo* findTorpedo(EntityId id) noexcept;
    [[nodiscard]] const Torpedo* findTorpedo(EntityId id) const noexcept;
    [[nodiscard]] const Base* findBase(EntityId id) const noexcept;
    [[nodiscard]] std::size_t activePlayerCount(TeamId team) const noexcept;
};

[[nodiscard]] const char* toString(ObjectKind value) noexcept;
[[nodiscard]] const char* toString(AccelerationMode value) noexcept;
[[nodiscard]] const char* toString(ShipClass value) noexcept;

} // namespace spasim
