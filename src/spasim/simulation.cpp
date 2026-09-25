#include "spasim/simulation.hpp"

#include "spasim/simulation_detail.hpp"

#include <type_traits>
#include <utility>

namespace spasim {

Simulation::Simulation(World world) : world_(std::move(world)) {}

void Simulation::step() {
    detail::updateMovement(world_);
    detail::updateTorpedoes(world_);
    detail::updateCombat(world_);
    detail::updateEconomy(world_);
    world_.elapsedSeconds += world_.fixedStepSeconds;
    ++world_.tick;
}

CommandResult Simulation::execute(PlayerId playerId, const WorldCommand& command) {
    auto* player = world_.findPlayer(playerId);
    if (!player) return {CommandStatus::NotFound, "Player not found."};

    auto* ship = world_.findShip(player->shipId);
    if (!ship) return {CommandStatus::NotFound, "Player ship not found."};

    return std::visit([&](const auto& action) -> CommandResult {
        using Action = std::decay_t<decltype(action)>;
        if constexpr (std::is_same_v<Action, TickWorld>) {
            step();
            return {CommandStatus::Applied, "Simulation advanced one fixed step."};
        } else {
            return detail::executeAction(world_, *player, *ship, action);
        }
    }, command);
}

void Simulation::writeSnapshot(std::ostream& output) const {
    detail::writeSnapshot(world_, output);
}

} // namespace spasim
