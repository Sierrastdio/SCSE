#include "engine/detail.hpp"

#include <algorithm>
#include <cmath>

namespace spasim::detail
{

CommandResult executeAction(World& world, Player& player, Ship& ship, const RepairVessel& action)
{
    if (!std::isfinite(action.amount) || action.amount <= 0.0)
        return {CommandStatus::Invalid, "Repair amount must be positive."};

    auto* base = world.findBase(player.baseId);
    if (!base)
        return {CommandStatus::NotFound, "Home base not found."};
    if (ship.position.distanceTo(base->position) > 100.0)
        return {CommandStatus::Invalid, "Return within 100 units of your base to repair."};

    const double repaired =
        std::min({action.amount, ship.combat.hullDamage, ship.resources.energy});
    ship.combat.hullDamage -= repaired;
    ship.resources.energy -= repaired;
    return {CommandStatus::Applied, "Ship repaired."};
}

CommandResult
executeAction(World& world, Player& player, Ship& ship, const TransferResources& action)
{
    if (action.target.kind != Target::Kind::Entity || action.target.id == player.shipId)
        return {CommandStatus::Invalid, "Choose another ship or base as transfer target."};

    auto* targetShip = world.findShip(action.target.id);
    auto* targetBase = world.findBase(action.target.id);
    if ((!targetShip && !targetBase) || !std::isfinite(action.amount) || action.amount <= 0.0)
        return {CommandStatus::Invalid, "Invalid transfer target or amount."};

    const Vec3& targetPosition = targetShip ? targetShip->position : targetBase->position;
    if (ship.position.distanceTo(targetPosition) > 100.0)
        return {CommandStatus::Invalid, "Transfer target is too far away."};

    Resources& destination = targetShip ? targetShip->resources : targetBase->resources;
    double& sourceValue = action.energy ? ship.resources.energy : ship.resources.rawMaterial;
    double& destinationValue = action.energy ? destination.energy : destination.rawMaterial;
    const double moved = std::min(sourceValue, action.amount);
    sourceValue -= moved;
    destinationValue += moved;
    return {CommandStatus::Applied, "Resources transferred."};
}

CommandResult executeAction(World& world, Player&, Ship& ship, const LoadFromPlanet& action)
{
    if (action.planet >= world.planets.size() || !std::isfinite(action.amount) ||
        action.amount <= 0.0)
        return {CommandStatus::Invalid, "Invalid planet or amount."};

    auto& planet = world.planets[action.planet];
    if (ship.position.distanceTo(planet.position) > 100.0)
        return {CommandStatus::Invalid, "Move within 100 units of the planet to load."};

    double& stock = action.energy ? planet.antiEntropy : planet.rawMaterial;
    double& cargo = action.energy ? ship.resources.energy : ship.resources.rawMaterial;
    const double loaded = std::min(stock, action.amount);
    stock -= loaded;
    cargo += loaded;
    return {CommandStatus::Applied, "Planet resources loaded."};
}

CommandResult executeAction(World& world, Player&, Ship&, const SetAlliances& action)
{
    if (action.first >= world.teams.size() || action.second >= world.teams.size())
        return {CommandStatus::Invalid, "Team ID is out of range."};

    auto setAlly = [&](Team& team, TeamId ally)
    {
        auto found = std::ranges::find(team.allies, ally);
        if (action.allied && found == team.allies.end())
            team.allies.push_back(ally);
        else if (!action.allied && found != team.allies.end())
            team.allies.erase(found);
    };
    setAlly(world.teams[action.first], action.second);
    setAlly(world.teams[action.second], action.first);
    return {CommandStatus::Applied, "Alliance updated."};
}

} // namespace spasim::detail
