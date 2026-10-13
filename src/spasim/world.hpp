#pragma once

#include "spasim/model.hpp"

#include <optional>
#include <string>

namespace spasim
{

[[nodiscard]] World makeInitialWorld(std::optional<std::uint64_t> seed = std::nullopt);
[[nodiscard]] std::optional<PlayerId> joinTeam(World& world, TeamId team, std::string callSign);
void removePlayer(World& world, PlayerId playerId);

} // namespace spasim
