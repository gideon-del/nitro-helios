#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
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

        TextureDesc depthTextureDesc;
        depthTextureDesc.size = {width, height};
        depthTextureDesc.usage = TextureDesc::Usage::DepthStencil | TextureDesc::Usage::RenderTarget;
        depthTextureDesc.format = TextureDesc::ImageFormat::Depth32Float;
        depthTexture = reinterpret_cast<MetalTexture *>(m_device->createTexture(depthTextureDesc));
    };

    void MetalSwapchain::resize(uint32_t newWidth, uint32_t newHeight)
    {
        if (newWidth == 0 || newHeight == 0)
            return;
        width = newWidth;
        height = newHeight;

        delete depthTexture;
        TextureDesc depthTextureDesc;
        depthTextureDesc.size = {width, height};
        depthTextureDesc.usage = TextureDesc::Usage::DepthStencil | TextureDesc::Usage::RenderTarget;
        depthTextureDesc.format = TextureDesc::ImageFormat::Depth32Float;
        depthTexture = reinterpret_cast<MetalTexture *>(m_device->createTexture(depthTextureDesc));
    }

    MetalSwapchain::~MetalSwapchain()
    {
        if (currentDrawable)
            currentDrawable->release();
        if (depthTexture)
            delete depthTexture;
        if (layer)
            layer->release();
    }
    uint32_t MetalSwapchain::getWidth()
    {

        return currentDrawable != nullptr ? currentDrawable->texture()->width() : 0;
    }
    uint32_t MetalSwapchain::getHeight()
    {
        return currentDrawable != nullptr ? currentDrawable->texture()->height() : 0;
    }
    RHITexture *MetalSwapchain::getCurrentBackbuffer()
    {
        return nullptr;
    }
}