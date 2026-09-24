#pragma once

#include "spasim/simulation.hpp"

#include <iosfwd>

namespace spasim {

class ConsoleApp {
public:
    ConsoleApp(Simulation& simulation, std::istream& input, std::ostream& output);
    int run();

private:
    void printHelp() const;
    void printStatus(PlayerId playerId) const;
    void printTeams() const;
    void printPlanets() const;

    Simulation& simulation_;
    std::istream& input_;
    std::ostream& output_;
};

} // namespace spasim
