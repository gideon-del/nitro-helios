#include <nitro-rhi-backends/vulkan/vulkan-swapchain.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-surface.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>

namespace nitro::rhi::vulkan
{
    VkSurfaceCapabilitiesKHR get_surface_capabilities(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface)
    {
        VkSurfaceCapabilitiesKHR capabilities;

        checkVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities), "Unable to fetch capabilites");

        return capabilities;
    };

    VkPresentModeKHR choose_present_mode(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface)
    {
        uint32_t presentCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, presentModes.data());

        for (const auto &mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR &capabilities, VulkanSurface *surface)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }

        VkExtent2D extent = {
            surface->getWidth(),
            surface->getHeight(),
        };

        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return extent;
    }

    VkSurfaceFormatKHR choose_surface_format(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface)
    {
        uint32_t formatCount;

        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        std::vector<VkSurfaceFormatKHR> formats(formatCount);

        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        for (const auto &format : formats)
        {

            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return format;
            }
        }

        return formats[0];
    }
    VulkanSwapchain::VulkanSwapchain(VulkanDevice *device) : m_device(device)
    {

        VkSurfaceCapabilitiesKHR capabilities = get_surface_capabilities(m_device->physicalDevice, m_device->surface->surface);
        VkPresentModeKHR presentMode = choose_present_mode(m_device->physicalDevice, m_device->surface->surface);
        format = choose_surface_format(m_device->physicalDevice, m_device->surface->surface);
        extent = choose_swap_extent(capabilities, m_device->surface);

        VkSwapchainCreateInfoKHR swapchainInfo{};

        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.imageColorSpace = format.colorSpace;
        swapchainInfo.imageFormat = format.format;
        swapchainInfo.presentMode = presentMode;
        swapchainInfo.imageExtent = extent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.surface = m_device->surface->surface;
        uint32_t imageCount = capabilities.minImageCount + 1;

        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }
        swapchainInfo.minImageCount = imageCount;

        checkVkResult(vkCreateSwapchainKHR(m_device->device, &swapchainInfo, nullptr, &swapchain), "Swapchain not created");

        uint32_t swapchainImageCount;

        vkGetSwapchainImagesKHR(m_device->device, swapchain, &swapchainImageCount, nullptr);
        std::vector<VkImage> images(swapchainImageCount);
        vkGetSwapchainImagesKHR(m_device->device, swapchain, &swapchainImageCount, images.data());

        for (auto image : images)
        {
            backBuffers.push_back(new VulkanTexture(m_device, image, extent.width, extent.height, format.format));
        }

        TextureDesc depthDesc;
        depthDesc.format = TextureDesc::ImageFormat::Depth32Float;
        depthDesc.size = {.width = extent.width, .height = extent.height};
        depthDesc.usage = TextureDesc::Usage::DepthStencil;
        depthTexture = new VulkanTexture(m_device, depthDesc);
        framebuffers.resize(images.size());

        for (int i = 0; i < backBuffers.size(); i++)
        {
            VkFramebufferCreateInfo framebufferInfo{};

            VkImageView attachments[] = {backBuffers[i]->imageView, depthTexture->imageView};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_device->defaultRenderPass;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.attachmentCount = 2;
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            checkVkResult(vkCreateFramebuffer(m_device->device, &framebufferInfo, nullptr, &framebuffers[i]), "Framebuffer not created");
        }

        imagesInFlight.resize(images.size(), VK_NULL_HANDLE);
        renderFinished.resize(images.size());

        for (int i = 0; i < images.size(); i++)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            checkVkResult(vkCreateSemaphore(m_device->device, &semaphoreInfo, nullptr, &renderFinished[i]), "Semaphore not created");
        }

        imageAvailable.resize(VulkanDevice::MAX_FRAMES_IN_FLIGHT);
        inFlight.resize(VulkanDevice::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < VulkanDevice::MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            checkVkResult(vkCreateSemaphore(m_device->device, &semaphoreInfo, nullptr, &imageAvailable[i]), "Semaphore not created");
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            checkVkResult(vkCreateFence(m_device->device, &fenceInfo, nullptr, &inFlight[i]), "Fence not created");
        }
    }

    void VulkanSwapchain::cleanup()
    {
        for (auto &fence : inFlight)
        {
            vkDestroyFence(m_device->device, fence, nullptr);
        }

        for (auto &renderSemaphore : renderFinished)
        {
            vkDestroySemaphore(m_device->device, renderSemaphore, nullptr);
        }
        for (auto &imageSemaphore : imageAvailable)
        {
            vkDestroySemaphore(m_device->device, imageSemaphore, nullptr);
        }
        for (auto &framebuffer : framebuffers)
        {
            vkDestroyFramebuffer(m_device->device, framebuffer, nullptr);
        }
        for (auto &backBuffer : backBuffers)
        {
            delete backBuffer;
        }

        vkDestroySwapchainKHR(m_device->device, swapchain, nullptr);
        inFlight.clear();
        renderFinished.clear();
        imageAvailable.clear();
        framebuffers.clear();
        backBuffers.clear();
    }
    VulkanSwapchain::~VulkanSwapchain()
    {
        cleanup();
    }

    RHITexture *VulkanSwapchain::getCurrentBackbuffer()
    {
        return backBuffers[currentImageIdx];
    }

    void VulkanSwapchain::resize(uint32_t width, uint32_t height)
    {

        if (width == 0 || height == 0)
            return;
        m_device->waitIdle();
        cleanup();
        VulkanSwapchain *newSwapchain = new VulkanSwapchain(m_device);

        swapchain = newSwapchain->swapchain;
        backBuffers = newSwapchain->backBuffers;
        framebuffers = newSwapchain->framebuffers;
        imagesInFlight = newSwapchain->imagesInFlight;
        renderFinished = newSwapchain->renderFinished;
        imageAvailable = newSwapchain->imageAvailable;
        inFlight = newSwapchain->inFlight;
        depthTexture = newSwapchain->depthTexture;
        format = newSwapchain->format;
        extent = newSwapchain->extent;

        newSwapchain->swapchain = VK_NULL_HANDLE;
        newSwapchain->depthTexture = nullptr;
        newSwapchain->backBuffers.clear();
        newSwapchain->framebuffers.clear();
        newSwapchain->inFlight.clear();
        newSwapchain->renderFinished.clear();
        newSwapchain->imageAvailable.clear();

        delete newSwapchain;
    }
}