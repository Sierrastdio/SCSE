#include "spasim/simulation_detail.hpp"

#include <ostream>

namespace spasim::detail
{

void writeSnapshot(const World& world, std::ostream& output)
{
    output << "{\"tick\":" << world.tick << ",\"time\":" << world.elapsedSeconds
           << ",\"entities\":[";

    bool firstEntity = true;
    const auto beginEntity = [&]
    {
        if (!firstEntity)
            output << ',';
        firstEntity = false;
    };

    for (const auto& slot : world.ships)
    {
        if (!slot || !slot->exists)
            continue;
        const auto& ship = *slot;
        beginEntity();
        output << "{\"id\":" << ship.id
               << ",\"kind\":\"ship\",\"team\":" << static_cast<int>(ship.team)
               << ",\"owner\":" << ship.owner << ",\"x\":" << ship.position.x
               << ",\"y\":" << ship.position.y << ",\"z\":" << ship.position.z
               << ",\"vx\":" << ship.velocity.x << ",\"vy\":" << ship.velocity.y
               << ",\"vz\":" << ship.velocity.z << ",\"orbit\":";
        if (ship.orbitingPlanet)
            output << static_cast<int>(*ship.orbitingPlanet);
        else
            output << "null";
        output << ",\"yaw\":" << ship.orientation.yawDegrees
               << ",\"pitch\":" << ship.orientation.pitchDegrees << ",\"speed\":" << ship.speed
               << ",\"radius\":" << ship.collisionRadius
               << ",\"destroyed\":" << (ship.combat.destroyed ? "true" : "false") << '}';
    }

    for (const auto& slot : world.bases)
    {
        if (!slot || !slot->exists)
            continue;
        const auto& base = *slot;
        beginEntity();
        output << "{\"id\":" << base.id
               << ",\"kind\":\"base\",\"team\":" << static_cast<int>(base.team)
               << ",\"owner\":" << base.owner << ",\"x\":" << base.position.x
               << ",\"y\":" << base.position.y << ",\"z\":" << base.position.z
               << ",\"radius\":" << base.collisionRadius
               << ",\"destroyed\":" << (base.combat.destroyed ? "true" : "false") << '}';
    }

    for (const auto& slot : world.torpedoes)
    {
        if (!slot || !slot->exists)
            continue;
        const auto& torpedo = *slot;
        beginEntity();
        output << "{\"id\":" << torpedo.id
               << ",\"kind\":\"torpedo\",\"team\":" << static_cast<int>(torpedo.team)
               << ",\"owner\":" << torpedo.owner << ",\"x\":" << torpedo.position.x
               << ",\"y\":" << torpedo.position.y << ",\"z\":" << torpedo.position.z
               << ",\"vx\":" << torpedo.velocity.x << ",\"vy\":" << torpedo.velocity.y
               << ",\"vz\":" << torpedo.velocity.z << ",\"radius\":" << torpedo.collisionRadius
               << '}';
    }

    output << "],\"planets\":[";
    for (std::size_t index = 0; index < world.planets.size(); ++index)
    {
        if (index != 0)
            output << ',';
        const auto& planet = world.planets[index];
        output << "{\"id\":" << static_cast<int>(planet.id) << ",\"name\":\"" << planet.name
               << "\",\"x\":" << planet.position.x << ",\"y\":" << planet.position.y
               << ",\"z\":" << planet.position.z << ",\"radius\":" << planet.radius
               << ",\"orbit_radius\":" << planet.radius + planet.parkingOrbitAltitude
               << ",\"mu\":" << planet.gravitationalParameter << '}';
    }
    output << "]}";
}

} // namespace spasim::detail
