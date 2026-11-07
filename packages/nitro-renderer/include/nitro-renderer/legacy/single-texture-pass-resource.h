#pragma once
#include <nitro-rhi/rhi.h>

namespace nitro::renderer
{
    struct SingleInputPassResource
    {
        rhi::RHIDescriptorSet *descriptorSet;
        rhi::RHITexture *lastInputTexture = nullptr;
    };
}