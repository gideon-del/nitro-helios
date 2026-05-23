#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <GLFW/glfw3.h>
namespace nitro::rhi::metal
{
    MetalSwapchain::MetalSwapchain(MetalDevice *device, void *windowHandle) : m_device(device)
    {
        auto rawLayer = createMetalLayer(windowHandle, device->device);

        layer = reinterpret_cast<CA::MetalLayer *>(rawLayer);

        GLFWwindow *window = reinterpret_cast<GLFWwindow *>(windowHandle);

        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        width = static_cast<uint32_t>(windowWidth);
        height = static_cast<uint32_t>(windowHeight);
        MTL::TextureDescriptor *desc = MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatDepth32Float,
            width,
            height,
            false);
        desc->setUsage(MTL::TextureUsageRenderTarget);
        desc->setStorageMode(MTL::StorageModePrivate);
        depthTexture = m_device->device->newTexture(desc);
        desc->release();
    };

    void MetalSwapchain::resize(uint32_t newWidth, uint32_t newHeight)
    {
        if (newWidth == 0 || newHeight == 0)
            return;
        width = newWidth;
        height = newHeight;

        depthTexture->release();
        MTL::TextureDescriptor *desc = MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatDepth32Float,
            width,
            height,
            false);
        desc->setUsage(MTL::TextureUsageRenderTarget);
        desc->setStorageMode(MTL::StorageModePrivate);
        depthTexture = m_device->device->newTexture(desc);
        desc->release();
    }

    MetalSwapchain::~MetalSwapchain()
    {
        if (currentDrawable)
            currentDrawable->release();
        if (depthTexture)
            depthTexture->release();
        if (layer)
            layer->release();
    }
    RHITexture *MetalSwapchain::getCurrentBackbuffer()
    {
        return nullptr;
    }
}