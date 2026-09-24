#include "spasim/console.hpp"

#include "spasim/world.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace spasim {

ConsoleApp::ConsoleApp(Simulation& simulation, std::istream& input, std::ostream& output)
    : simulation_(simulation), input_(input), output_(output) {}

void ConsoleApp::printHelp() const {
    output_ << "Commands: status, teams, planets, turn <yaw> <pitch>, accelerate, brake, goal <1-4>, fire, tick, quit\n"
               "The 3D terminal view and some legacy commands are not implemented yet.\n";
}

void ConsoleApp::printStatus(PlayerId playerId) const {
    const auto* player = simulation_.world().findPlayer(playerId);
    if (!player) return;
    const auto* ship = simulation_.world().findShip(player->shipId);
    if (!ship) return;
    output_ << std::fixed << std::setprecision(2)
            << player->callSign << " | " << simulation_.world().teams[player->team].name
            << " | position (" << ship->position.x << ", " << ship->position.y << ", " << ship->position.z << ")"
            << " | yaw/pitch " << ship->orientation.yawDegrees << '/' << ship->orientation.pitchDegrees
            << " | warp " << ship->warp << " (" << toString(ship->acceleration) << ")"
            << " | energy " << ship->resources.energy << " | shields " << ship->combat.shields
            << " | hull damage " << ship->combat.hullDamage << "\n";
}

void ConsoleApp::printTeams() const {
    for (const auto& team : simulation_.world().teams) {
        output_ << static_cast<int>(team.id + 1) << ". " << team.name << " ("
                << simulation_.world().activePlayerCount(team.id) << '/' << kPlayersPerTeam << ")\n";
    }
}

void ConsoleApp::printPlanets() const {
    for (const auto& planet : simulation_.world().planets) {
        output_ << static_cast<int>(planet.id + 1) << ". " << planet.name << " at ("
                << planet.position.x << ", " << planet.position.y << ", " << planet.position.z
                << ") | population " << planet.population << " | energy " << planet.antiEntropy
                << " | raw material " << planet.rawMaterial << " | SOL " << planet.standardOfLiving
                << " | skill " << planet.skill << "\n";
    }
}

int ConsoleApp::run() {
    output_ << "SPASIM C++ port scaffold\nChoose team (1-4): ";
    std::string line;
    if (!std::getline(input_, line)) return 0;
    int teamNumber{};
    try { teamNumber = std::stoi(line); } catch (...) { teamNumber = 0; }
    if (teamNumber < 1 || teamNumber > static_cast<int>(kTeamCount)) {
        output_ << "Invalid team.\n";
        return 1;
    }
    output_ << "Call sign (1-20 characters): ";
    if (!std::getline(input_, line)) return 0;
    const auto playerId = joinTeam(simulation_.world(), static_cast<TeamId>(teamNumber - 1), line);
    if (!playerId) {
        output_ << "Could not join the selected team.\n";
        return 1;
    }
    output_ << "Welcome, " << line << ". Type help for commands.\n";

    while (output_ << "> " && std::getline(input_, line)) {
        const auto command = parseCommand(line);
        if (command.kind == CommandKind::Quit) break;
        switch (command.kind) {
        case CommandKind::Help: printHelp(); break;
        case CommandKind::Status: printStatus(*playerId); break;
        case CommandKind::Teams: printTeams(); break;
        case CommandKind::Planets: printPlanets(); break;
        default: output_ << simulation_.execute(*playerId, command) << '\n'; break;
        }
    }
    removePlayer(simulation_.world(), *playerId);
    return 0;
}

} // namespace spasim
