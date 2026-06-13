#pragma once
#include <nitro-rhi/rhi-swapchain.h>
#include <vulkan/vulkan.h>
#include "vulkan-texture.h"
#include <vector>
namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanSwapchain : public RHISwapchain
    {
    public:
        ~VulkanSwapchain() override;
        VulkanSwapchain(VulkanDevice *device);
        RHITexture *getCurrentBackbuffer() override;
        void resize(uint32_t width, uint32_t height) override;
        uint32_t getWidth() override;
        uint32_t getHeight() override;
        RHIViewScale getViewScale() override { return m_viewScale; }
        void cleanup();
        VkSwapchainKHR swapchain;
        std::vector<VulkanTexture *> backBuffers;
        std::vector<VkFramebuffer> framebuffers;

        std::vector<VkFence> imagesInFlight;
        std::vector<VkSemaphore> renderFinished;

        uint32_t currentImageIdx = 0;

        VulkanTexture *depthTexture = nullptr;
        VkSurfaceFormatKHR format;
        VkExtent2D extent;

    private:
        VulkanDevice *m_device;
        RHIViewScale m_viewScale{1.0f, 1.0f};
    };

}
