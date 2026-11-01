#pragma once

#include "engine/types.hpp"

#include <string>
#include <variant>

namespace spasim
{

struct SetHeading
{
    Vec3 direction{};
};

struct SetAcceleration
{
    AccelerationMode mode{AccelerationMode::Cruise};
};

struct SetThrottleLevel
{
    std::uint8_t level{2};
};

struct SetGoal
{
    Target target{};
    std::uint8_t orbitLevel{1};
};

struct SetOrbitLevel
{
    std::uint8_t level{1};
};

struct SetShipClass
{
    ShipClass shipClass{ShipClass::Small};
};

struct FirePhasor
{
    double power{};
};

struct SelectTarget
{
    Target target{};
};

struct FireTorpedo
{
    double energy{};
    Vec3 aim{};
};

struct DetonateTorpedo
{
};

struct SetWeaponProfile
{
    WeaponId weapon{};
};

struct RepairVessel
{
    double amount{};
};

struct TransferResources
{
    Target target{};
    bool energy{};
    double amount{};
};

struct LoadFromPlanet
{
    PlanetId planet{};
    bool energy{};
    double amount{};
};

struct TickWorld
{
};

struct SetAlliances
{
    TeamId first{};
    TeamId second{};
    bool allied{};
};

using WorldCommand = std::variant<SetHeading,
                                  SetAcceleration,
                                  SetThrottleLevel,
                                  SetGoal,
                                  SetOrbitLevel,
                                  SetShipClass,
                                  SelectTarget,
                                  FirePhasor,
                                  FireTorpedo,
                                  DetonateTorpedo,
                                  SetWeaponProfile,
                                  RepairVessel,
                                  TransferResources,
                                  LoadFromPlanet,
                                  SetAlliances,
                                  TickWorld>;

enum class CommandStatus : std::uint8_t
{
    Applied,
    Invalid,
    NotFound,
    Unsupported
};

struct CommandResult
{
    CommandStatus status{CommandStatus::Applied};
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == CommandStatus::Applied;
    }
};

} // namespace spasim
