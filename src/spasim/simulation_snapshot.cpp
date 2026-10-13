#include "spasim/simulation_detail.hpp"

#include <iomanip>
#include <ostream>
#include <string_view>

namespace spasim::detail
{
namespace
{
void writeString(std::ostream& output, std::string_view value)
{
    output << '"';
    for (const unsigned char character : value)
        switch (character)
        {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (character < 0x20)
                output << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<int>(character) << std::dec << std::setfill(' ');
            else
                output << static_cast<char>(character);
        }
    output << '"';
}

void writeVector(std::ostream& output, const Vec3& vector)
{
    output << '[' << vector.x << ',' << vector.y << ',' << vector.z << ']';
}
} // namespace

void writeSnapshot(const World& world, std::ostream& output)
{
    output << "{\"tick\":" << world.tick << ",\"time\":" << world.elapsedSeconds
           << ",\"fixed_step\":" << world.fixedStepSeconds << ",\"teams\":[";
    for (std::size_t index = 0; index < world.teams.size(); ++index)
    {
        if (index)
            output << ',';
        const auto& team = world.teams[index];
        output << "{\"id\":" << static_cast<int>(team.id) << ",\"name\":";
        writeString(output, team.name);
        output << ",\"home_planet\":" << static_cast<int>(team.homePlanet)
               << ",\"class\":";
        writeString(output, toString(team.shipClass));
        output << ",\"random_spawn\":" << (team.randomSpawn ? "true" : "false")
               << ",\"spawn_radius\":" << team.spawnRadius << ",\"spawn_offset\":";
        writeVector(output, team.spawnOffset);
        output << ",\"base_offset\":";
        writeVector(output, team.baseOffset);
        output << ",\"starting_orbit_level\":"
               << static_cast<int>(team.startingOrbitLevel) << '}';
    }
    output << "],\"entities\":[";

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
               << ",\"team_name\":";
        writeString(output, world.teams[ship.team].name);
        output << ",\"home_planet\":" << static_cast<int>(world.teams[ship.team].homePlanet)
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
               << ",\"class\":\"" << toString(ship.shipClass)
               << "\",\"orbit_level\":" << static_cast<int>(ship.orbitLevel)
               << ",\"radius\":" << ship.collisionRadius << ",\"spawn_origin\":";
        if (ship.team < world.teams.size() && world.teams[ship.team].homePlanet < world.planets.size())
        {
            const auto& origin = world.planets[world.teams[ship.team].homePlanet].position;
            const auto offset = ship.position - origin;
            writeVector(output, offset);
            output << ",\"spawn_distance\":" << offset.length();
        }
        else
            output << "null,\"spawn_distance\":null";
        output << ",\"destroyed\":" << (ship.combat.destroyed ? "true" : "false") << '}';
    }

    for (const auto& slot : world.bases)
    {
        if (!slot || !slot->exists)
            continue;
        const auto& base = *slot;
        beginEntity();
        output << "{\"id\":" << base.id
               << ",\"kind\":\"base\",\"team\":" << static_cast<int>(base.team)
               << ",\"team_name\":";
        writeString(output, world.teams[base.team].name);
        output << ",\"home_planet\":" << static_cast<int>(base.homePlanet)
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
               << ",\"team_name\":";
        writeString(output, world.teams[torpedo.team].name);
        output << ",\"owner\":" << torpedo.owner << ",\"x\":" << torpedo.position.x
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
        output << "{\"id\":" << static_cast<int>(planet.id) << ",\"name\":";
        writeString(output, planet.name);
        output << ",\"x\":" << planet.position.x << ",\"y\":" << planet.position.y
               << ",\"z\":" << planet.position.z << ",\"radius\":" << planet.radius
               << ",\"orbit_radius\":" << planet.radius + planet.parkingOrbitAltitude
               << ",\"sphere_radius\":" << planet.sphereOfInfluenceRadius << ",\"orbit_levels\":[";
        for (std::uint8_t level = 0; level < planet.orbitLevelCount; ++level)
        {
            if (level > 0)
                output << ',';
            output << planet.orbitLevelRadii[level];
        }
        output << "],\"mu\":" << planet.gravitationalParameter
               << ",\"population\":" << planet.population << ",\"energy\":"
               << planet.antiEntropy << ",\"material\":" << planet.rawMaterial
               << ",\"standard_of_living\":" << planet.standardOfLiving << ",\"skill\":"
               << planet.skill << ",\"controlling_team_id\":";
        if (planet.controllingTeam)
            output << static_cast<int>(*planet.controllingTeam);
        else
            output << "null";
        output << '}';
    }
    output << "]}";
}

} // namespace spasim::detail
