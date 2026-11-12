#pragma once
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-deffered-deletion-queue.h>
#include <nitro-rhi/rhi-pool.h>
#include <nitro-core/types/pool.h>
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.h>

#include <optional>
#include "vulkan-surface.h"
#include <vector>
#include "vulkan-sampler.h"
#include "vulkan-sampler-cache.h"
#include <source_location>

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
    class VulkanSwapchain;
    class VulkanTexture;
    class VulkanBuffer;
    class VulkanHeap;
    class VulkanDevice : public RHIDevice
    {

    public:
        ~VulkanDevice();
        VulkanDevice(void *window);

        RHISwapchain *createSwapchain(RHISurface *surface) override;

        RHIDescriptorLayout *createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings, std::source_location loc = std::source_location::current()) override;
        void destroyDescriptorLayout(RHIDescriptorLayout *layout) override;

        RHIDescriptorSet *createDescriptorSet(RHIDescriptorLayout *layout, std::source_location loc = std::source_location::current()) override;
        void destroyDescriptorSet(RHIDescriptorSet *set) override;

        RHITimer *createTimer() override;
        void destroyTimer(RHITimer *timer) override;
        RHIRenderPass *createRenderPass(const RenderPassDesc &desc) override;
        void destroyRenderPass(RHIRenderPass *renderPass) override;

        RHIBufferHandle create(const BufferDesc &desc) override;
        RHIBuffer *resolve(const RHIBufferHandle &handle) override;
        void destroy(const RHIBufferHandle &handle) override;

        RHIBuffer *createBuffer(const BufferDesc &desc) override;
        void destroyBuffer(RHIBuffer *buffer) override;

        RHITextureHandle create(const TextureDesc &desc) override;
        RHITexture *resolve(const RHITextureHandle &handle) override;
        void destroy(const RHITextureHandle &handle) override;

        RHITexture *createTexture(const TextureDesc &desc) override;
        void destroyTexture(RHITexture *texture) override;

        RHIPipeline *createPipeline(const PipelineDesc &desc) override;
        void destroyPipeline(RHIPipeline *pipeline) override;

        RHIComputePipeline *createComputePipeline(const ComputePipelineDesc &desc) override;
        void destroyComputePipeline(RHIComputePipeline *pipeline) override;
        uint32_t getCurrentFrameIndex() const override;

        RHISamplerHandle create(const RHISamplerDesc &desc) override;
        void destroy(RHISamplerHandle &handle) override;

        void *getImGuiTextureRef(RHITexture *texture) override;
        MemoryRequirements textureMemoryRequirements(const TextureDesc &desc) override;
        const DefaultSamplers &defaultSamplers() const override { return m_defaultSamplers; };
        const VulkanSampler &get(RHISamplerHandle handle) { return m_samplers.get(handle.id); }
        void beginImGuiFrame() override;
        void endImGuiFrame() override;
        void drawImGui(RHICommandBuffer *cmd) override;
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

        VkCommandBuffer beginOneTimeCommands();
        void endOneTimeCommands(VkCommandBuffer &cmd);
        RHICommandBuffer *createCommandBuffer() override;
        RHICommandBuffer *beginFrame() override;
        void endFrame(RHICommandBuffer *cmd) override;
        void endCommandBuffer(RHICommandBuffer *cmd) override;
        void waitIdle() override;

        RHIHeapHandle create(size_t sizeBytes, uint32_t memoryTypeBits) override;
        RHIHeap *resolve(const RHIHeapHandle &handle) override;
        void destroy(const RHIHeapHandle &handle) override;

        RHIHeap *createHeap(size_t sizeBytes, uint32_t memoryTypeBits) override;
        void destroyHeap(RHIHeap *heap) override;

        RHITexture *createTextureFromHeap(RHIHeap *heap, const TextureDesc &desc, size_t offset) override;
        RHITextureHandle createTextureFromHeap(const RHIHeapHandle &heapHandle, const TextureDesc &desc, size_t offset) override;

        VkFormat getSurfaceFormat() const;
        bool supportsSamplerComparison() const { return m_mutableComparisonSamplers; };
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

    private:
        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_messageCallback;

        QueueFamilyIndices m_queueFamilyIndices;
        VkFormat m_surfaceFormat;
        std::vector<std::unique_ptr<VulkanCommandBuffer>> m_frames;
        uint32_t m_currentFrame = 0;
        uint64_t m_frameNumber = 0;
        VulkanSwapchain *m_swapchain;
        RHIPool<VulkanSampler> m_samplers;
        VulkanSamplerCache m_samplerCache;
        DefaultSamplers m_defaultSamplers;
        std::unordered_map<RHITextureHandle, VkDescriptorSet, RHITextureHandleHash> m_imguiTextureCache;
        bool m_mutableComparisonSamplers = false;

        ResourcePool<std::unique_ptr<VulkanTexture>, RHITextureTag> m_texturePool;
        ResourcePool<std::unique_ptr<VulkanBuffer>, RHIBufferTag> m_bufferPool;
        ResourcePool<std::unique_ptr<VulkanHeap>, RHIHeapTag> m_heapPool;

        DeferredDeletionQueue<std::unique_ptr<VulkanTexture>> m_textureGraveyard;
        DeferredDeletionQueue<std::unique_ptr<VulkanBuffer>> m_bufferGraveyard;
        DeferredDeletionQueue<std::unique_ptr<VulkanHeap>> m_heapGraveyard;

        std::unordered_map<RHIDescriptorSet *, std::source_location> m_liveSets;
        std::unordered_map<RHIDescriptorLayout *, std::source_location> m_liveDescriptorLayouts;

    private:
        void checkMutableComparisonSampler();
        void collectGarbage();
    };
}