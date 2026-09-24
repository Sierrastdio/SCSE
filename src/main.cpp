#include "spasim/console.hpp"
#include "spasim/simulation.hpp"
#include "spasim/world.hpp"

#include <iostream>

int main() {
    spasim::Simulation simulation{spasim::makeInitialWorld()};
    spasim::ConsoleApp app{simulation, std::cin, std::cout};
    return app.run();
}
