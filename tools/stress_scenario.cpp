#include "engine/engine.hpp"
#include "engine/world.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace
{

double processCpuSeconds()
{
#ifdef _WIN32
  FILETIME creationTime{};
  FILETIME exitTime{};
  FILETIME kernelTime{};
  FILETIME userTime{};
  if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime) == 0)
  {
    return 0.0;
  }
  ULARGE_INTEGER kernel{};
  ULARGE_INTEGER user{};
  kernel.LowPart = kernelTime.dwLowDateTime;
  kernel.HighPart = kernelTime.dwHighDateTime;
  user.LowPart = userTime.dwLowDateTime;
  user.HighPart = userTime.dwHighDateTime;
  return static_cast<double>(kernel.QuadPart + user.QuadPart) / 10'000'000.0;
#else
  return 0.0;
#endif
}

void printProcessMemory()
{
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX counters{};
  counters.cb = sizeof(counters);
  if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters), sizeof(counters)) != 0)
  {
    constexpr double megabytes = 1024.0 * 1024.0;
    std::cout << " | Working set: " << static_cast<double>(counters.WorkingSetSize) / megabytes << " MB"
              << " | Private: " << static_cast<double>(counters.PrivateUsage) / megabytes << " MB";
  }
#endif
}

} // namespace

int main(int argc, char **argv)
{
  constexpr std::size_t planetCount = 100;
  constexpr std::size_t shipCount = 300;
  constexpr double pi = 3.14159265358979323846;
  const auto durationSeconds = argc > 1 ? std::stoull(argv[1]) : 300ULL;
  const auto reportIntervalSeconds = argc > 2 ? std::stoull(argv[2]) : 1ULL;

  spasim::Simulation simulation{spasim::makeInitialWorld()};
  auto &world = simulation.world();
  const auto planetTemplate = world.planets.front();
  world.planets.clear();
  world.planets.reserve(planetCount);
  for (std::size_t i = 0; i < planetCount; ++i)
  {
    auto planet = planetTemplate;
    planet.id = static_cast<spasim::PlanetId>(i);
    planet.name = "Load planet " + std::to_string(i + 1);
    planet.position = {static_cast<double>(i % 10) * 5000.0, static_cast<double>(i / 10) * 5000.0, 0.0};
    world.planets.push_back(std::move(planet));
  }

  for (std::size_t i = 0; i < shipCount; ++i)
  {
    const auto &planet = world.planets[i % planetCount];
    const double angle = 2.0 * pi * static_cast<double>(i / planetCount) / 3.0;
    const double radius = 250.0 + 50.0 * static_cast<double>(i / planetCount);
    const double speed = std::sqrt(planet.gravitationalParameter / radius);
    auto ship = world.practiceShipDefaults;
    ship.name = "Load ship " + std::to_string(i + 1);
    ship.team = static_cast<spasim::TeamId>(i % world.teams.size());
    ship.playerControlled = true;
    ship.owner = spasim::kNoPlayer;
    ship.position = planet.position + spasim::Vec3{radius * std::cos(angle), radius * std::sin(angle), 0.0};
    ship.velocity = {-speed * std::sin(angle), speed * std::cos(angle), 0.0};
    (void)world.spawnShip(std::move(ship));
  }

  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  auto lastReport = start;
  auto lastCpuSeconds = processCpuSeconds();
  std::uint64_t tickCount{};
  std::cout << "Stress run: " << world.planets.size() << " planets, " << shipCount << " moving ships for " << durationSeconds
            << " seconds. Press Ctrl+C to stop.\n";
  while (std::chrono::duration<double>(clock::now() - start).count() < static_cast<double>(durationSeconds))
  {
    simulation.step();
    ++tickCount;
    const auto now = clock::now();
    const double reportElapsed = std::chrono::duration<double>(now - lastReport).count();
    if (reportElapsed >= static_cast<double>(reportIntervalSeconds))
    {
      const double cpuSeconds = processCpuSeconds();
      const double cpuPercent = reportElapsed > 0.0 ? (cpuSeconds - lastCpuSeconds) / reportElapsed * 100.0 : 0.0;
      const double wallElapsed = std::chrono::duration<double>(now - start).count();
      std::cout << "Time: " << wallElapsed << " s | Ticks: " << tickCount << " | CPU: " << cpuPercent << "% (one core = 100%)";
      printProcessMemory();
      std::cout << '\n' << std::flush;
      lastReport = now;
      lastCpuSeconds = cpuSeconds;
    }
  }
  const auto elapsed = std::chrono::duration<double>(clock::now() - start).count();
  const double simulatedSeconds = static_cast<double>(tickCount) * world.fixedStepSeconds;
  std::cout << "Finished. Planets: " << world.planets.size() << " | Ships: " << shipCount << " | Ticks: " << tickCount << '\n'
            << "Simulated time: " << simulatedSeconds << " s\n"
            << "Wall time: " << elapsed << " s\n"
            << "Simulation rate: " << (elapsed > 0.0 ? simulatedSeconds / elapsed : 0.0) << " simulated s / wall s\n";
}
