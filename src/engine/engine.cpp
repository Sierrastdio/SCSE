#include "engine/engine.hpp"

#include "engine/detail.hpp"

#include <type_traits>
#include <stdexcept>
#include <utility>

namespace spasim
{

Simulation::Simulation(World world, std::filesystem::path rulesPath)
    : world_(std::move(world)), gameRules_(rulesPath)
{
    gameRules_.applyScenario(world_);
    for (auto& planet : world_.planets)
    {
        planet.orbitLevelCount = gameRules_.levelCount;
        for (std::uint8_t level = 1; level <= gameRules_.levelCount; ++level)
        {
            planet.orbitLevelRadii[level - 1] = gameRules_.orbitRadius(planet, level);
            if (planet.orbitLevelRadii[level - 1] < planet.radius + planet.parkingOrbitAltitude)
                throw std::runtime_error(
                    "Orbit rules place an orbit inside the planet's safe orbit.");
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

CommandResult Simulation::execute(PlayerId playerId, const WorldCommand& command)
{
    auto* player = world_.findPlayer(playerId);
    if (!player)
        return {CommandStatus::NotFound, "Player not found."};

    auto* ship = world_.findShip(player->shipId);
    if (!ship)
        return {CommandStatus::NotFound, "Player ship not found."};

    return std::visit(
        [&](const auto& action) -> CommandResult
        {
            using Action = std::decay_t<decltype(action)>;
            if constexpr (std::is_same_v<Action, TickWorld>)
            {
                step();
                return {CommandStatus::Applied, "Simulation advanced one fixed step."};
            }
            else if constexpr (std::is_same_v<Action, SetGoal> ||
                               std::is_same_v<Action, SetOrbitLevel> ||
                               std::is_same_v<Action, SetShipClass> ||
                               std::is_same_v<Action, SelectTarget>)
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
