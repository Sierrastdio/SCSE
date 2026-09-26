#pragma once

#include "spasim/simulation.hpp"

#include <iosfwd>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <string>

namespace spasim
{

class ConsoleApp
{
  public:
    ConsoleApp(Simulation& simulation, std::istream& input, std::ostream& output);
    int run();

  private:
    void printHelp() const;
    void printStatus(PlayerId playerId) const;
    void printTeams() const;
    void printPlanets() const;
    [[nodiscard]] WorldCommand toWorldCommand(const Command& command, PlayerId playerId) const;
    void inputWorker();
    [[nodiscard]] bool pollLine(std::string& line);
    void dispatch(PlayerId playerId, const Command& command);
    void writeSnapshotFile() const;

    Simulation& simulation_;
    std::istream& input_;
    std::ostream& output_;
    std::queue<std::string> inputQueue_;
    std::mutex inputMutex_;
    std::atomic_bool inputFinished_{};
    bool snapshotEnabled_{};
};

} // namespace spasim
