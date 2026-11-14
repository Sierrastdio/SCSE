#include "engine/engine.hpp"

#include "engine/detail.hpp"

#include <algorithm>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace spasim
{
namespace
{

std::vector<EntityId> selectedShipIds(const Player &player)
{
  return player.selectedShips.empty() ? std::vector<EntityId>{player.shipId} : player.selectedShips;
}

bool isControlledShip(const Ship *ship, PlayerId playerId)
{
  return ship && ship->exists && !ship->combat.destroyed && ship->playerControlled && ship->owner == playerId;
}

} // namespace

Simulation::Simulation(World world, std::filesystem::path rulesPath) : world_(std::move(world)), gameRules_(rulesPath)
{
  gameRules_.applyScenario(world_);
  for (auto &planet : world_.planets)
  {
    planet.orbitLevelCount = gameRules_.levelCount;
    for (std::uint8_t level = 1; level <= gameRules_.levelCount; ++level)
    {
      planet.orbitLevelRadii[level - 1] = gameRules_.orbitRadius(planet, level);
      if (planet.orbitLevelRadii[level - 1] < planet.radius + planet.parkingOrbitAltitude)
        throw std::runtime_error("Orbit rules place an orbit inside the planet's safe orbit.");
    }
  }
}

void Simulation::step()
{
  detail::updateMovement(world_, gameRules_);
  detail::updateTorpedoes(world_);
  world_.elapsedSeconds += world_.fixedStepSeconds;
  ++world_.tick;
}

CommandResult Simulation::execute(PlayerId playerId, const WorldCommand &command)
{
  auto *player = world_.findPlayer(playerId);
  if (!player)
    return {CommandStatus::NotFound, "Player not found."};

  auto *ship = world_.findShip(player->shipId);
  if (!ship)
    return {CommandStatus::NotFound, "Player ship not found."};

  return std::visit(
    [&](const auto &action) -> CommandResult {
      using Action = std::decay_t<decltype(action)>;
      if constexpr (std::is_same_v<Action, TickWorld>)
      {
        step();
        return {CommandStatus::Applied, "Simulation advanced one fixed step."};
      }
      else if constexpr (std::is_same_v<Action, SelectShips>)
      {
        if (action.ships.empty())
          return {CommandStatus::Invalid, "Select at least one ship."};

        std::vector<EntityId> selection;
        selection.reserve(action.ships.size());
        for (const auto shipId : action.ships)
        {
          auto *selectedShip = world_.findShip(shipId);
          if (!selectedShip || !selectedShip->exists || selectedShip->combat.destroyed)
            return {CommandStatus::NotFound, "Selected ship does not exist or is destroyed."};
          if (!selectedShip->playerControlled || selectedShip->owner != playerId)
            return {CommandStatus::Invalid, "You can only select ships under your control."};
          if (std::find(selection.begin(), selection.end(), shipId) == selection.end())
            selection.push_back(shipId);
        }

        player->selectedShips = std::move(selection);
        return {CommandStatus::Applied, "Selected " + std::to_string(player->selectedShips.size()) + " ship(s)."};
      }
      else if constexpr (std::is_same_v<Action, SelectTarget>)
      {
        if (action.target.kind != Target::Kind::Entity)
          return {CommandStatus::Invalid, "Select requires an entity target."};
        const auto *targetPosition = detail::positionOf(world_, action.target);
        if (!targetPosition)
          return {CommandStatus::NotFound, "Selectable entity not found."};

        const auto selection = selectedShipIds(*player);
        for (const auto selectedId : selection)
        {
          const auto *selectedShip = world_.findShip(selectedId);
          if (!isControlledShip(selectedShip, playerId))
            return {CommandStatus::Invalid, "A selected ship is no longer under your control."};
          if ((*targetPosition - selectedShip->position).length() <= 1e-9)
            return {CommandStatus::Invalid, "Cannot aim at an entity at a selected ship's exact position."};
        }

        for (const auto selectedId : selection)
        {
          auto *selectedShip = world_.findShip(selectedId);
          const auto result = detail::executeAction(world_, *player, *selectedShip, action, gameRules_);
          if (!result)
            return result;
        }
        return {CommandStatus::Applied, "Target assigned to " + std::to_string(selection.size()) + " ship(s)."};
      }
      else if constexpr (
        std::is_same_v<Action, SetHeading> || std::is_same_v<Action, SetAcceleration> || std::is_same_v<Action, SetThrottleLevel>)
      {
        const auto selection = selectedShipIds(*player);
        for (const auto selectedId : selection)
          if (!isControlledShip(world_.findShip(selectedId), playerId))
            return {CommandStatus::Invalid, "A selected ship is no longer under your control."};

        for (const auto selectedId : selection)
        {
          auto *selectedShip = world_.findShip(selectedId);
          const auto result = detail::executeAction(world_, *player, *selectedShip, action);
          if (!result)
            return result;
        }
        return {CommandStatus::Applied, "Command applied to " + std::to_string(selection.size()) + " selected ship(s)."};
      }
      else if constexpr (std::is_same_v<Action, FirePhasor>)
      {
        const auto selection = selectedShipIds(*player);
        std::size_t firedCount{};
        CommandResult lastResult{CommandStatus::Invalid, "No selected ship could fire."};
        for (const auto selectedId : selection)
        {
          auto *selectedShip = world_.findShip(selectedId);
          if (!isControlledShip(selectedShip, playerId))
            continue;
          lastResult = detail::executeAction(world_, *player, *selectedShip, action);
          if (lastResult)
            ++firedCount;
        }
        if (firedCount == 0)
          return lastResult;
        return {CommandStatus::Applied,
          "Weapon fired from " + std::to_string(firedCount) + " of " + std::to_string(selection.size()) + " selected ship(s)."};
      }
      else if constexpr (std::is_same_v<Action, FireTorpedo> || std::is_same_v<Action, DetonateTorpedo>)
      {
        const auto selection = selectedShipIds(*player);
        std::size_t completedCount{};
        CommandResult lastResult{CommandStatus::Invalid, std::is_same_v<Action, FireTorpedo>
                                                           ? "No selected ship could launch a torpedo."
                                                           : "No selected ship had an active torpedo."};
        for (const auto selectedId : selection)
        {
          auto *selectedShip = world_.findShip(selectedId);
          if (!isControlledShip(selectedShip, playerId))
            continue;
          lastResult = detail::executeAction(world_, *player, *selectedShip, action);
          if (lastResult)
            ++completedCount;
        }
        if (completedCount == 0)
          return lastResult;
        const std::string actionName = std::is_same_v<Action, FireTorpedo> ? "Torpedoes launched from " : "Torpedoes detonated on ";
        return {CommandStatus::Applied,
          actionName + std::to_string(completedCount) + " of " + std::to_string(selection.size()) + " selected ship(s)."};
      }
      else if constexpr (std::is_same_v<Action, SetGoal>)
      {
        const auto selection = selectedShipIds(*player);
        if (action.target.kind != Target::Kind::Planet || action.target.id >= world_.planets.size())
          return {CommandStatus::Invalid, "Navigation goals must target a planet."};

        for (const auto selectedId : selection)
        {
          const auto *selectedShip = world_.findShip(selectedId);
          if (!isControlledShip(selectedShip, playerId))
            return {CommandStatus::Invalid, "A selected ship is no longer under your control."};
          if (action.orbitLevel == 0 || action.orbitLevel > gameRules_.maximumLevel(selectedShip->shipClass))
            return {CommandStatus::Invalid, "The selected orbit level is not supported by every selected ship."};
        }

        for (const auto selectedId : selection)
        {
          auto *selectedShip = world_.findShip(selectedId);
          const auto result = detail::executeAction(world_, *player, *selectedShip, action, gameRules_);
          if (!result)
            return result;
        }
        return {CommandStatus::Applied, "Navigation order applied to " + std::to_string(selection.size()) + " selected ship(s)."};
      }
      else if constexpr (std::is_same_v<Action, SetOrbitLevel>)
      {
        const auto selection = selectedShipIds(*player);
        for (const auto selectedId : selection)
        {
          const auto *selectedShip = world_.findShip(selectedId);
          if (!isControlledShip(selectedShip, playerId))
            return {CommandStatus::Invalid, "A selected ship is no longer under your control."};
          if (!selectedShip->orbitingPlanet || *selectedShip->orbitingPlanet >= world_.planets.size())
            return {CommandStatus::Invalid, "All selected ships must be in orbit before changing orbit level."};
          if (action.level == 0 || action.level > gameRules_.maximumLevel(selectedShip->shipClass))
            return {CommandStatus::Invalid, "The selected orbit level is not supported by every selected ship."};
        }

        for (const auto selectedId : selection)
        {
          auto *selectedShip = world_.findShip(selectedId);
          const auto result = detail::executeAction(world_, *player, *selectedShip, action, gameRules_);
          if (!result)
            return result;
        }
        return {CommandStatus::Applied, "Orbit transfer applied to " + std::to_string(selection.size()) + " selected ship(s)."};
      }
      else if constexpr (std::is_same_v<Action, SetShipClass>)
      {
        return detail::executeAction(world_, *player, *ship, action, gameRules_);
      }
      else
      {
        return detail::executeAction(world_, *player, *ship, action);
      }
    },
    command);
}

} // namespace spasim
