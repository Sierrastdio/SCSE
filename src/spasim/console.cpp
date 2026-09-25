#include "spasim/console.hpp"

#include "spasim/world.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <numbers>
#include <sstream>
#include <thread>

namespace spasim {

ConsoleApp::ConsoleApp(Simulation& simulation, std::istream& input, std::ostream& output)
    : simulation_(simulation), input_(input), output_(output) {}

void ConsoleApp::printHelp() const {
    output_ << "Commands: status, teams, planets, turn <x> <y> <z>, accelerate, brake, cruise, goal <planet 1-4>, "
               "select <entity-id>, weapon <1-4>, fire [power], torpedo <energy>, detonate, repair <amount>, "
               "load <energy|material> <amount> <planet 1-4>, transfer <energy|material> <amount> <entity-id>, "
               "ally <team 1-4> <team 1-4>, pause, resume, quit\n"
               "Select accepts an entity ID (select 2), ship-2, ship #2, or a practice target alias T1-T4.\n"
               "It aims at a ship; fire hits only when it is in range and in front.\n"
               "turn rotates the nose only; accelerate thrusts along the nose, brake opposes velocity, cruise coasts.\n"
               "goal <planet> navigates toward a planet; crossing its parking orbit inserts the ship into orbit.\n"
               "Planets exert gravity; status separates nose direction from inertial velocity and shows engine mode.\n"
               "The simulation advances automatically in real time. Prefix a command with tick to advance once.\n";
}

void ConsoleApp::printStatus(PlayerId playerId) const {
    const auto* player = simulation_.world().findPlayer(playerId);
    const auto* ship = player ? simulation_.world().findShip(player->shipId) : nullptr;
    if (!player || !ship) return;
    const double yaw = ship->orientation.yawDegrees * std::numbers::pi / 180.0;
    const double pitch = ship->orientation.pitchDegrees * std::numbers::pi / 180.0;
    output_ << std::fixed << std::setprecision(2)
            << player->callSign << " | " << simulation_.world().teams[player->team].name
            << " | pos (" << ship->position.x << ", " << ship->position.y << ", " << ship->position.z << ")"
            << " | facing (" << std::cos(pitch) * std::cos(yaw) << ", "
            << std::cos(pitch) * std::sin(yaw) << ", " << std::sin(pitch) << ")"
            << " | velocity (" << ship->velocity.x << ", " << ship->velocity.y << ", " << ship->velocity.z << ")"
            << " | heading angles " << ship->orientation.yawDegrees << '/' << ship->orientation.pitchDegrees
            << " | engines " << toString(ship->acceleration) << " | speed " << ship->speed
            << " | energy " << ship->resources.energy
            << " | shields " << ship->combat.shields << " | hull " << ship->combat.hull - ship->combat.hullDamage
            << " | sim " << simulation_.world().elapsedSeconds << "s\n";
    if (ship->orbitingPlanet && *ship->orbitingPlanet < kPlanetCount)
        output_ << "Orbiting: " << simulation_.world().planets[*ship->orbitingPlanet].name << "\n";
    else if (player->goal && player->goal->kind == Target::Kind::Planet && player->goal->id < kPlanetCount)
        output_ << "Navigation goal: " << simulation_.world().planets[player->goal->id].name << "\n";
    if (player->selectedTarget) output_ << "Combat target: entity " << *player->selectedTarget << "\n";
}

void ConsoleApp::printTeams() const {
    const auto& world = simulation_.world();
    output_ << "Ships (select by ID, e.g. 'select 2'; practice targets are stationary):\n";
    for (const auto& team : simulation_.world().teams) {
        output_ << static_cast<int>(team.id + 1) << ". " << team.name << " ("
                << world.activePlayerCount(team.id) << '/' << kPlayersPerTeam << ")\n";
        for (const auto& slot : world.ships) {
            if (!slot || !slot->exists || slot->team != team.id) continue;
            const auto& ship = *slot;
            output_ << "  ship id=" << ship.id;
            if (!ship.playerControlled) output_ << " alias=T" << static_cast<int>(ship.team + 1);
            output_ << " \"" << ship.name << "\""
                    << (ship.playerControlled ? " [player]" : " [stationary practice target]")
                    << " at (" << std::fixed << std::setprecision(0) << ship.position.x << ", "
                    << ship.position.y << ", " << ship.position.z << ")"
                    << " shields=" << std::setprecision(0) << ship.combat.shields
                    << " hull=" << ship.combat.hull - ship.combat.hullDamage << '\n';
        }
        for (const auto& slot : world.bases) {
            if (!slot || !slot->exists || slot->team != team.id) continue;
            output_ << "  base id=" << slot->id << " at (" << std::fixed << std::setprecision(0)
                    << slot->position.x << ", " << slot->position.y << ", " << slot->position.z << ")\n";
        }
    }
}

void ConsoleApp::printPlanets() const {
    for (const auto& planet : simulation_.world().planets) {
        output_ << static_cast<int>(planet.id + 1) << ". " << planet.name << " at ("
                << planet.position.x << ", " << planet.position.y << ", " << planet.position.z
                << ") | population " << planet.population << " | energy " << planet.antiEntropy
                << " | raw material " << planet.rawMaterial << " | SOL " << planet.standardOfLiving
                << " | skill " << planet.skill << '\n';
    }
}

WorldCommand ConsoleApp::toWorldCommand(const Command& command, PlayerId playerId) const {
    switch (command.kind) {
    case CommandKind::Turn: return SetHeading{{command.first, command.second, command.thirdValue}};
    case CommandKind::Accelerate: return SetAcceleration{AccelerationMode::Accelerate};
    case CommandKind::Brake: return SetAcceleration{AccelerationMode::Decelerate};
    case CommandKind::Cruise: return SetAcceleration{AccelerationMode::Cruise};
    case CommandKind::SetGoal: return SetGoal{Target{Target::Kind::Planet, command.target}};
    case CommandKind::Select: return SelectTarget{Target{Target::Kind::Entity, command.target}};
    case CommandKind::Fire: return FirePhasor{command.hasFirst ? command.first : 1.0};
    case CommandKind::FireTorpedo: {
        const auto* player = simulation_.world().findPlayer(playerId);
        const auto* ship = player ? simulation_.world().findShip(player->shipId) : nullptr;
        const double yaw = ship ? ship->orientation.yawDegrees * std::numbers::pi / 180.0 : 0.0;
        const double pitch = ship ? ship->orientation.pitchDegrees * std::numbers::pi / 180.0 : 0.0;
        return FireTorpedo{command.first,
            {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)}};
    }
    case CommandKind::Detonate: return DetonateTorpedo{};
    case CommandKind::SetWeapon: return SetWeaponProfile{static_cast<WeaponKind>(command.target)};
    case CommandKind::Repair: return RepairVessel{command.first};
    case CommandKind::Transfer:
        return TransferResources{Target{Target::Kind::Entity, command.target}, command.text == "energy", command.first};
    case CommandKind::Load:
        return LoadFromPlanet{static_cast<PlanetId>(command.target - 1), command.text == "energy", command.first};
    case CommandKind::Tick: return TickWorld{};
    default: return TickWorld{};
    }
}

void ConsoleApp::inputWorker() {
    std::string line;
    while (std::getline(input_, line)) {
        const bool shouldQuit = line == "quit" || line == "exit";
        {
            std::scoped_lock lock(inputMutex_);
            inputQueue_.push(std::move(line));
        }
        if (shouldQuit) break;
    }
    inputFinished_.store(true);
}

bool ConsoleApp::pollLine(std::string& line) {
    std::scoped_lock lock(inputMutex_);
    if (inputQueue_.empty()) return false;
    line = std::move(inputQueue_.front());
    inputQueue_.pop();
    return true;
}

void ConsoleApp::dispatch(PlayerId playerId, const Command& command) {
    auto execute = [&](const WorldCommand& action) {
        const auto result = simulation_.execute(playerId, action);
        if (!result.message.empty()) output_ << result.message << '\n';
    };
    switch (command.kind) {
    case CommandKind::Help: printHelp(); break;
    case CommandKind::Status: printStatus(playerId); break;
    case CommandKind::Teams: printTeams(); break;
    case CommandKind::Planets: printPlanets(); break;
    case CommandKind::Turn:
        if (!command.hasFirst || !command.hasSecond || !command.hasThird)
            output_ << "Usage: turn <x> <y> <z> (non-zero direction vector)\n";
        else execute(toWorldCommand(command, playerId));
        break;
    case CommandKind::Accelerate:
    case CommandKind::Brake:
    case CommandKind::Cruise:
    case CommandKind::Fire:
    case CommandKind::FireTorpedo:
    case CommandKind::Detonate:
    case CommandKind::Repair:
    case CommandKind::Transfer:
    case CommandKind::Load:
    case CommandKind::Select:
        if (command.kind == CommandKind::Fire && command.hasFirst && command.first <= 0.0) {
            output_ << "Power must be positive.\n"; break;
        }
        if ((command.kind == CommandKind::FireTorpedo || command.kind == CommandKind::Repair) && !command.hasFirst) {
            output_ << "An amount is required.\n"; break;
        }
        if ((command.kind == CommandKind::Transfer || command.kind == CommandKind::Load) &&
            (!command.hasFirst || !command.hasTarget || (command.text != "energy" && command.text != "material"))) {
            output_ << "Usage: " << (command.kind == CommandKind::Load ? "load <energy|material> <amount> <planet 1-4>" :
                                     "transfer <energy|material> <amount> <entity-id>") << '\n'; break;
        }
        if (command.kind == CommandKind::Select && !command.hasTarget) {
            output_ << "Usage: select <entity-id> (e.g. select 2, select ship-2, or select T2; see teams)\n";
            break;
        }
        if (command.kind == CommandKind::Load && (command.target < 1 || command.target > kPlanetCount)) {
            output_ << "Planet must be 1-4.\n"; break;
        }
        execute(toWorldCommand(command, playerId));
        break;
    case CommandKind::SetGoal:
        if (!command.hasTarget) output_ << "Usage: goal <planet 1-4>\n";
        else execute(toWorldCommand(command, playerId));
        break;
    case CommandKind::SetWeapon:
        if (!command.hasTarget) output_ << "Usage: weapon <1-4>\n";
        else execute(toWorldCommand(command, playerId));
        break;
    case CommandKind::Ally: {
        if (!command.hasFirst || !command.hasSecond || command.first < 1 || command.first > 4 || command.second < 1 || command.second > 4) {
            output_ << "Usage: ally <team 1-4> <team 1-4>\n"; break;
        }
        execute(SetAlliances{static_cast<TeamId>(command.first - 1), static_cast<TeamId>(command.second - 1), true});
        break;
    }
    case CommandKind::Pause: simulation_.world().paused = true; output_ << "Simulation paused.\n"; break;
    case CommandKind::Resume: simulation_.world().paused = false; output_ << "Simulation resumed.\n"; break;
    case CommandKind::Snapshot:
        snapshotEnabled_ = command.text != "off";
        if (snapshotEnabled_) {
            writeSnapshotFile();
            output_ << "Live snapshots enabled at debug/world.json\n";
        } else output_ << "Live snapshots disabled.\n";
        break;
    case CommandKind::Tick: execute(TickWorld{}); break;
    case CommandKind::Unknown: output_ << "Unknown command. Type help.\n"; break;
    case CommandKind::Quit: break;
    }
}

void ConsoleApp::writeSnapshotFile() const {
    std::error_code error;
    std::filesystem::create_directories("debug", error);
    if (error) {
        output_ << "Could not create debug directory: " << error.message() << '\n';
        return;
    }
    std::ofstream snapshot("debug/world.json", std::ios::trunc);
    if (!snapshot) {
        output_ << "Could not write debug/world.json.\n";
        return;
    }
    simulation_.writeSnapshot(snapshot);
    snapshot << '\n';
}

int ConsoleApp::run() {
    output_ << "SPASIM — 3D space simulation, C++ port\nChoose team (1-4): ";
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
    output_ << "Welcome, " << line << ". Simulation running. Type help.\n> ";
    std::thread reader(&ConsoleApp::inputWorker, this);
    using clock = std::chrono::steady_clock;
    auto nextStep = clock::now();
    bool quit = false;
    while (!quit) {
        if (clock::now() >= nextStep) {
            if (!simulation_.world().paused) simulation_.step();
            if (snapshotEnabled_) writeSnapshotFile();
            const auto stepDuration = std::chrono::duration_cast<clock::duration>(
                std::chrono::duration<double>(simulation_.world().fixedStepSeconds));
            nextStep += stepDuration;
            if (clock::now() - nextStep > stepDuration * 4) nextStep = clock::now();
        }
        bool printed = false;
        while (pollLine(line)) {
            const auto command = parseCommand(line);
            if (command.kind == CommandKind::Quit) { quit = true; break; }
            if (command.kind == CommandKind::Tick) simulation_.step();
            else dispatch(*playerId, command);
            printed = true;
        }
        if (printed && !quit) output_ << "> ";
        if (inputFinished_.load() && !quit) quit = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (reader.joinable()) reader.join();
    removePlayer(simulation_.world(), *playerId);
    return 0;
}

} // namespace spasim
