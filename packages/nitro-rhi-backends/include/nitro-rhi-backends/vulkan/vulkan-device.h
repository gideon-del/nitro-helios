#pragma once
#include <nitro-rhi/rhi-device.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <optional>
#include "vulkan-surface.h"
#include "vulkan-buffer.h"
#include "vulkan-texture.h"

namespace nitro::rhi::vulkan

{

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    class VulkanDevice : public RHIDevice
    {

    public:
        ~VulkanDevice();
        VulkanDevice(void *window);

        RHIBuffer *createBuffer(const BufferDesc &desc) override;
        void destroyBuffer(RHIBuffer *buffer) override;

        RHITexture *createTexture(const TextureDesc &desc) override;
        void destroyTexture(RHITexture *texture) override;

        RHIPipeline *createPipeline(const PipelineDesc &desc) override;
        void destroyPipeline(RHIPipeline *pipeline) override;
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        void copyBufferToImage(VkBuffer &src, VkImage &image, VkDeviceSize size, VkExtent3D imageExtent);

        void transitionImageLayout(
            VkCommandBuffer &cmd,
            VkImage image,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            VkPipelineStageFlags srcStage,
            VkPipelineStageFlags dstStage,
            VkImageAspectFlags aspectMask);

        VkCommandBuffer beginOneTimeCommands();
        void endOneTimeCommands(VkCommandBuffer &cmd);

        VkDevice device;
        VmaAllocator allocator;
        VkCommandPool commandPool;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VulkanSurface *surface;
        VkRenderPass defaultRenderPass;

    private:
        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_messageCallback;
        VkPhysicalDevice m_physicalDevice;
        QueueFamilyIndices m_queueFamilyIndices;
        VkFormat m_surfaceFormat;
    };
}