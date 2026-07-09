#pragma once
#include <nitro-rhi/rhi.h>

namespace nitro::renderer
{
    rhi::RHITexture *loadHDRImage(std::shared_ptr<rhi::RHIDevice> device, std::string filePath);
    rhi::RHITexture *createCubeMap(std::shared_ptr<rhi::RHIDevice> device, rhi::RHITexture *sourceTexture, uint32_t size, std::string shaderDir, bool isMetal);
} // namespace nitro::renderer
