#pragma once
#include "nitro-renderer/graph/resource-types.h"
#include <span>

namespace nitro::renderer
{
    AliasPlan planAliases(std::span<const TransientEntry> entries);
} // namespace nitro::renderer
