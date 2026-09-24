#pragma once

#include "spasim/model.hpp"

#include <optional>
#include <string>

namespace spasim {

[[nodiscard]] World makeInitialWorld(std::uint64_t seed = 0x53504153494DULL);
[[nodiscard]] std::optional<PlayerId> joinTeam(World& world, TeamId team, std::string callSign);
void removePlayer(World& world, PlayerId playerId);

} // namespace spasim
