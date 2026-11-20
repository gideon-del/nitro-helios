#include "nitro-rhi-backends/vulkan/vulkan-descriptor-allocator.h"
#include "nitro-rhi-backends/vulkan/vulkan-device.h"
#include "nitro-rhi-backends/vulkan/vulkan-utils.h"

namespace nitro::rhi::vulkan
{
    VulkanFrameDescriptorAllocator::VulkanFrameDescriptorAllocator(VulkanDevice &device) : m_device(device) {}

    VulkanFrameDescriptorAllocator::~VulkanFrameDescriptorAllocator()
    {
        for (auto &framePool : m_frames)
        {
            for (auto &pool : framePool.used)
                vkDestroyDescriptorPool(m_device.device, pool, nullptr);
        }

        for (auto &pool : m_freePools)
            vkDestroyDescriptorPool(m_device.device, pool, nullptr);
    }

    RHIDescriptorSet *VulkanFrameDescriptorAllocator::allocate(VulkanDescriptorLayout *layout, uint32_t frameIdx)
    {
        assert(frameIdx < kMaxFramesInFlight && "Must be less than Max Frames in flight");
        auto &frame = m_frames[frameIdx];

        if (frame.current == VK_NULL_HANDLE)
        {
            acquirePool(frame);
        }

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = frame.current;
        allocateInfo.descriptorSetCount = 1;

        allocateInfo.pSetLayouts = &layout->descriptorSetLayout;

        VkDescriptorSet vkSet = VK_NULL_HANDLE;
        VkResult result = vkAllocateDescriptorSets(
            m_device.device,
            &allocateInfo,
            &vkSet);

        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
        {
            acquirePool(frame);
            allocateInfo.descriptorPool = frame.current;
            result = vkAllocateDescriptorSets(
                m_device.device,
                &allocateInfo,
                &vkSet);
        }
        checkVkResult(result, "Failed to allocate frame descriptor set");

        if (frame.wrapperCount == frame.wrappers.size())
            frame.wrappers.push_back(std::make_unique<VulkanDescriptorSet>(&m_device, layout, vkSet, false));
        else
            frame.wrappers[frame.wrapperCount]->reset(layout, vkSet);
        return frame.wrappers[frame.wrapperCount++].get();
    }

    VkDescriptorPool VulkanFrameDescriptorAllocator::createPool()
    {
        const VkDescriptorPoolSize sizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 512},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 256},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 512},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256},
        };

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.flags = 0;
        info.maxSets = 512;
        info.poolSizeCount = uint32_t(std::size(sizes));
        info.pPoolSizes = sizes;
        VkDescriptorPool pool;
        checkVkResult(vkCreateDescriptorPool(m_device.device, &info, nullptr, &pool), "frame descriptor pool");
        return pool;
    }
    VkDescriptorPool VulkanFrameDescriptorAllocator::acquirePool(FrameDescriptorPools &frame)
    {
        VkDescriptorPool pool;
        if (!m_freePools.empty())
        {
            pool = m_freePools.back();
            m_freePools.pop_back();
        }
        else
        {
            pool = createPool();
        }
        frame.used.push_back(pool);
        frame.current = pool;
        return pool;
    }

    void VulkanFrameDescriptorAllocator::reset(uint32_t frameIdx)
    {
        assert(frameIdx < kMaxFramesInFlight && "Must be less than Max Frames in flight");
        auto &frame = m_frames[frameIdx];
        frame.wrapperCount = 0;
        frame.current = VK_NULL_HANDLE;
        for (auto &pool : frame.used)
            vkResetDescriptorPool(m_device.device, pool, 0);

        m_freePools.insert(m_freePools.end(), frame.used.begin(), frame.used.end());
        frame.used.clear();
    }
} // namespace nitro::rhi::vulkan
