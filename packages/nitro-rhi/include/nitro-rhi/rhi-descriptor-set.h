#pragma once
#include "rhi-buffer.h"
#include "rhi-texture.h"
namespace nitro::rhi
{

    class RHIDescriptorSet
    {
    public:
        virtual ~RHIDescriptorSet() = default;
        virtual void writeBuffer(RHIBuffer *buffer, uint32_t binding) = 0;
        virtual void writeTexture(RHITexture *texture, uint32_t binding) = 0;
        virtual void commit() = 0;
    };
} // namespace nitro::rhi
