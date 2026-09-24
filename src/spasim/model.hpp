#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace spasim {

inline constexpr std::size_t kTeamCount = 4;
inline constexpr std::size_t kPlayersPerTeam = 6;
inline constexpr std::size_t kPlayerCapacity = kTeamCount * kPlayersPerTeam;
inline constexpr std::size_t kPlanetCount = 4;
inline constexpr std::size_t kWeaponCount = 4;

using PlayerId = std::uint32_t;
using EntityId = std::uint32_t;
using PlanetId = std::uint8_t;
using TeamId = std::uint8_t;

struct Vec3 {
    double x{};
    double y{};
    double z{};

    [[nodiscard]] double length() const noexcept;
    [[nodiscard]] double distanceTo(const Vec3& other) const noexcept;
    Vec3& operator+=(const Vec3& other) noexcept;
    Vec3& operator-=(const Vec3& other) noexcept;
};

[[nodiscard]] Vec3 operator+(Vec3 lhs, const Vec3& rhs) noexcept;
[[nodiscard]] Vec3 operator-(Vec3 lhs, const Vec3& rhs) noexcept;
[[nodiscard]] Vec3 operator*(Vec3 value, double scalar) noexcept;

struct Orientation {
    double yawDegrees{};
    double pitchDegrees{};
};

enum class ObjectKind : std::uint8_t { Ship, Base, Torpedo };
enum class WeaponKind : std::uint8_t { Aggstroms, Fouriers, Diffractions, Lasers };
enum class AccelerationMode : std::uint8_t { Decelerate, Cruise, Accelerate };

struct Resources {
    double energy{};
    double rawMaterial{};
};

struct CombatState {
    double hull{};
    double hullDamage{};
    double shields{};
    double skill{};
    bool destroyed{};
};

struct Ship {
    EntityId id{};
    PlayerId owner{};
    TeamId team{};
    Vec3 position{};
    Orientation orientation{};
    double speed{};
    double warp{};
    AccelerationMode acceleration{AccelerationMode::Cruise};
    Resources resources{};
    CombatState combat{};
    bool exists{true};
};

struct Base {
    EntityId id{};
    PlayerId owner{};
    TeamId team{};
    PlanetId homePlanet{};
    Vec3 position{};
    Resources resources{};
    CombatState combat{};
    bool exists{true};
};

struct Torpedo {
    EntityId id{};
    PlayerId owner{};
    TeamId team{};
    Vec3 position{};
    Vec3 velocity{};
    double energy{};
    double warp{};
    double fieldStrength{};
    bool controlled{};
    bool detonated{};
    bool exists{};
};

struct Player {
    PlayerId id{};
    TeamId team{};
    std::string callSign;
    std::string station;
    EntityId shipId{};
    EntityId baseId{};
    EntityId torpedoId{};
    EntityId selectedObject{};
    std::optional<EntityId> goal;
    double phasorPower{1.0};
    double phasorDiameter{1.0};
    WeaponKind weapon{WeaponKind::Aggstroms};
    bool phasorEnabled{};
    bool joined{};
};

struct Planet {
    PlanetId id{};
    std::string name;
    Vec3 position{};
    double population{};
    double antiEntropy{};
    double rawMaterial{};
    double standardOfLiving{};
    double skill{};
    std::optional<TeamId> controllingTeam;
    double revoltAtSeconds{};
};

struct Team {
    TeamId id{};
    std::string name;
    std::array<bool, kTeamCount> allied{};
};

struct World {
    std::array<Team, kTeamCount> teams{};
    std::array<Planet, kPlanetCount> planets{};
    std::array<std::optional<Player>, kPlayerCapacity> playerSlots{};
    std::vector<Ship> ships;
    std::vector<Base> bases;
    std::vector<Torpedo> torpedoes;
    std::uint64_t tick{};
    double elapsedSeconds{};
    double fixedStepSeconds{0.1};
    std::mt19937_64 random{0x53504153494DULL};
    EntityId nextEntityId{1};

    [[nodiscard]] const Player* findPlayer(PlayerId id) const noexcept;
    [[nodiscard]] Player* findPlayer(PlayerId id) noexcept;
    [[nodiscard]] Ship* findShip(EntityId id) noexcept;
    [[nodiscard]] const Ship* findShip(EntityId id) const noexcept;
    [[nodiscard]] Base* findBase(EntityId id) noexcept;
    [[nodiscard]] Torpedo* findTorpedo(EntityId id) noexcept;
    [[nodiscard]] std::size_t activePlayerCount(TeamId team) const noexcept;
};

[[nodiscard]] const char* toString(ObjectKind value) noexcept;
[[nodiscard]] const char* toString(WeaponKind value) noexcept;
[[nodiscard]] const char* toString(AccelerationMode value) noexcept;

} // namespace spasim
