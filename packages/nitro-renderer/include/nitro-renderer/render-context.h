#pragma once
#include "nitro-rhi/rhi-device.h"

namespace nitro::renderer
{
    struct RenderContext
    {
        rhi::RHIDevice &device;
    };
} // namespace nitro::renderer
