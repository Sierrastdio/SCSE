#include "console/terminal.hpp"
#include "engine/engine.hpp"
#include "engine/world.hpp"

#include <iostream>

int main()
{
    spasim::Simulation simulation{spasim::makeInitialWorld()};
    spasim::ConsoleApp app{simulation, std::cin, std::cout};
    return app.run();
}
