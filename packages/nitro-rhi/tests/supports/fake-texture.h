#pragma once
#include "nitro-rhi/rhi-texture.h"

namespace nitro::tests
{
    using namespace rhi;
    class FakeTexture : public RHITexture
    {
    public:
        FakeTexture(uint32_t mips, uint32_t layers) { m_resourceState = TextureState(mips, layers); }
    };
}