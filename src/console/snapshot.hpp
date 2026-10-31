#pragma once

#include "engine/types.hpp"

#include <iosfwd>

namespace spasim::console
{

void writeSnapshot(const World& world, std::ostream& output);

} // namespace spasim::console
