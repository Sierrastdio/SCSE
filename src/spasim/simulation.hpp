#pragma once

#include "spasim/command.hpp"
#include "spasim/model.hpp"

#include <string>
#include <iosfwd>
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
struct SetGoal
{
    Target target{};
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
    WeaponKind weapon{WeaponKind::Aggstroms};
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
                                  SetGoal,
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

class Simulation
{
  public:
    explicit Simulation(World world);

    [[nodiscard]] World& world() noexcept
    {
        return world_;
    }
    [[nodiscard]] const World& world() const noexcept
    {
        return world_;
    }
    void step();
    [[nodiscard]] CommandResult execute(PlayerId playerId, const WorldCommand& command);
    void writeSnapshot(std::ostream& output) const;

  private:
    World world_;
};

} // namespace spasim
