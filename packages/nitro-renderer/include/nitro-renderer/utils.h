#pragma once
#include <nitro-rhi/rhi.h>

namespace nitro::renderer
{
    rhi::RHITexture *loadHDRImage(std::shared_ptr<rhi::RHIDevice> device, std::string filePath);
    rhi::RHITexture *createCubeMap(std::shared_ptr<rhi::RHIDevice> device, rhi::RHITexture *sourceTexture, uint32_t size, std::string shaderDir, bool isMetal);
    rhi::RHITexture *generateIrradianceMap(
        std::shared_ptr<rhi::RHIDevice> device, rhi::RHITexture *environment, uint32_t size, std::string shaderDir, bool isMetal);
    rhi::RHITexture *generatePrefliteredMap(
        std::shared_ptr<rhi::RHIDevice> device, rhi::RHITexture *environment, uint32_t size, std::string shaderDir, bool isMetal);
    rhi::RHITexture *generateBrdfLUT(
        std::shared_ptr<rhi::RHIDevice> device, uint32_t size, std::string shaderDir, bool isMetal);

    rhi::RHITexture *generateSSAONoiseTexture(std::shared_ptr<rhi::RHIDevice> device);

} // namespace nitro::renderer
