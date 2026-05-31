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

    private:
        MetalDevice *m_device;
    };
} // namespace nitro::rhi::metal
