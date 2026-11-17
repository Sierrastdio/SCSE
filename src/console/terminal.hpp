#pragma once

#include "console/command_parser.hpp"
#include "engine/engine.hpp"

#include <atomic>
#include <iosfwd>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace spasim
{

class ConsoleApp
{
public:
  ConsoleApp(Simulation &simulation, std::istream &input, std::ostream &output);
  int run();

private:
  void printHelp() const;
  void printStatus(PlayerId playerId) const;
  void printTeams() const;
  void printPlanets() const;
  [[nodiscard]] WorldCommand toWorldCommand(const Command &command, PlayerId playerId) const;
  void inputWorker();
  [[nodiscard]] bool pollLine(std::string &line);
  void dispatch(PlayerId playerId, const Command &command);

  Simulation &simulation_;
  std::istream &input_;
  std::ostream &output_;
  std::queue<std::string> inputQueue_;
  std::mutex inputMutex_;
  std::atomic_bool inputFinished_{};
  double timeScale_{1.0};
};

} // namespace spasim
