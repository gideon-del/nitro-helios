#pragma once
#include <nitro-rhi/rhi-render-pass.h>
#include <SingleHeader/MetalCpp.h>

namespace nitro::rhi::metal
{
    class MetalTexture;
    class MetalDevice;
    class MetalRenderPass : public RHIRenderPass
    {
    public:
        MetalRenderPass(MetalDevice *device, const RenderPassDesc &desc);
        ~MetalRenderPass() override;
        MTL::RenderPassDescriptor *renderPassDescriptor;
        MetalTexture *depthTexture = nullptr;
        MetalTexture *colorTexture = nullptr;

        uint32_t width;
        uint32_t height;

    private:
        MetalDevice *m_device;
    };
} // namespace nitro::rhi::metal
