#pragma once

#include "spasim/simulation.hpp"

namespace spasim::detail
{

// Shared simulation math and world lookups used by individual systems.
[[nodiscard]] Vec3 forwardVector(const Orientation& orientation);
[[nodiscard]] Orientation orientationToward(const Vec3& direction);
[[nodiscard]] CombatState* combatState(World& world, Target target);
[[nodiscard]] std::optional<TeamId> teamOf(const World& world, EntityId id);
[[nodiscard]] Vec3* positionOf(World& world, Target target);
[[nodiscard]] Vec3 gravityAcceleration(const World& world, const Vec3& position);

// Command handlers are grouped by the simulation subsystem they affect.
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const SetHeading&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const SetAcceleration&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const SetThrottleLevel&);
[[nodiscard]] CommandResult
executeAction(World&, Player&, Ship&, const SetGoal&, const GameRules&);
[[nodiscard]] CommandResult
executeAction(World&, Player&, Ship&, const SetOrbitLevel&, const GameRules&);
[[nodiscard]] CommandResult
executeAction(World&, Player&, Ship&, const SetShipClass&, const GameRules&);
[[nodiscard]] CommandResult
executeAction(World&, Player&, Ship&, const SelectTarget&, const GameRules&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const FirePhasor&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const FireTorpedo&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const DetonateTorpedo&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const SetWeaponProfile&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const RepairVessel&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const TransferResources&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const LoadFromPlanet&);
[[nodiscard]] CommandResult executeAction(World&, Player&, Ship&, const SetAlliances&);

void updateMovement(World& world, const GameRules& rules);
void updateTorpedoes(World& world);
void writeSnapshot(const World& world, std::ostream& output);

} // namespace spasim::detail
