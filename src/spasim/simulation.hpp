#pragma once

#include "spasim/command.hpp"
#include "spasim/model.hpp"

#include <string>

namespace spasim {

class Simulation {
public:
    explicit Simulation(World world);

    [[nodiscard]] World& world() noexcept { return world_; }
    [[nodiscard]] const World& world() const noexcept { return world_; }
    void step();
    [[nodiscard]] std::string execute(PlayerId playerId, const Command& command);

private:
    void updateMovement();
    void updateTorpedoes();
    void updatePlanetEconomy();

    World world_;
};

} // namespace spasim
