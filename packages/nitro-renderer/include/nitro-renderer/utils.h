#pragma once
#include <nitro-rhi/rhi.h>

namespace nitro::renderer
{
    rhi::RHITexture *loadHDRImage(std::shared_ptr<rhi::RHIDevice> device, std::string filePath);
} // namespace nitro::renderer
