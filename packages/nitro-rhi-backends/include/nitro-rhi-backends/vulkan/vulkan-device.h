#pragma once
#include <nitro-rhi/rhi-device.h>
#include <vulkan/vulkan.h>
#include <optional>
#include "vulkan-surface.h"
#include <vector>

struct VmaAllocator_T;
typedef VmaAllocator_T *VmaAllocator;

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

    class VulkanCommandBuffer;
    class VulkanDevice : public RHIDevice
    {

    public:
        ~VulkanDevice();
        VulkanDevice(void *window);

        RHISwapchain *createSwapchain(RHISurface *surface) override;

        RHIDescriptorLayout *createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings) override;
        void destroyDescriptorLayout(RHIDescriptorLayout *layout) override;

        RHIDescriptorSet *createDescriptorSet(RHIDescriptorLayout *layout) override;
        void destroyDescriptorSet(RHIDescriptorSet *set) override;

        RHITimer *createTimer() override;
        RHIRenderPass *createRenderPass(const RenderPassDesc &desc) override;
        void destroyRenderPass(RHIRenderPass *renderPass) override;
        RHIBuffer *createBuffer(const BufferDesc &desc) override;
        void destroyBuffer(RHIBuffer *buffer) override;

        RHITexture *createTexture(const TextureDesc &desc) override;
        void destroyTexture(RHITexture *texture) override;

        RHIPipeline *createPipeline(const PipelineDesc &desc) override;
        void destroyPipeline(RHIPipeline *pipeline) override;

        RHIComputePipeline *createComputePipeline(const ComputePipelineDesc &desc) override;
        void destroyComputePipeline(RHIComputePipeline *pipeline) override;
        uint32_t getCurrentFrameIndex() const override;

        void beginImGuiFrame() override;
        void endImGuiFrame() override;
        void drawImGui(RHICommandBuffer *cmd) override;
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

        RHICommandBuffer *beginFrame() override;
        void endFrame(RHICommandBuffer *cmd) override;
        void waitIdle();
        VkFormat getSurfaceFormat() const;
        VkDevice device;
        VmaAllocator allocator;
        VkCommandPool commandPool;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VulkanSurface *surface;
        VkRenderPass defaultRenderPass;
        VkPhysicalDevice physicalDevice;
        VkQueryPool queryPool;
        float timestampPeriod;

        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    private:
        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_messageCallback;

        QueueFamilyIndices m_queueFamilyIndices;
        VkFormat m_surfaceFormat;
        std::vector<VulkanCommandBuffer *> m_frames;
        uint32_t m_currentFrame = 0;
    };
}