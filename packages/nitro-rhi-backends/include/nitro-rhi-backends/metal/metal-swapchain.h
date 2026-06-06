#pragma once
#include <nitro-rhi/rhi-swapchain.h>
#include <SingleHeader/MetalCpp.h>
extern "C" void *createMetalLayer(void *glfwWindow, void *mtlDevice);
namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalTexture;
    class MetalSwapchain : public RHISwapchain
    {
    public:
        MetalSwapchain(MetalDevice *device, void *windowHandle);
        ~MetalSwapchain() override;

        void resize(uint32_t width, uint32_t height) override;
        RHITexture *getCurrentBackbuffer() override;
        uint32_t getWidth() override;
        uint32_t getHeight() override;

        CA::MetalLayer *layer;
        CA::MetalDrawable *currentDrawable = nullptr;

        MetalTexture *depthTexture;
        uint32_t width;
        uint32_t height;

    private:
        MetalDevice *m_device;
    };
}