#pragma once
#include "nitro-frame/resources/resource-types.h"
#include <span>

namespace nitro::frame
{
    AliasPlan planAliases(std::span<const TransientEntry> entries);
} // namespace nitro::frame
