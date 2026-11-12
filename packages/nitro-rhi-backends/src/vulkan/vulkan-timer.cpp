// vulkan-timer.cpp
#include <nitro-rhi-backends/vulkan/vulkan-timer.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-command-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include "nitro-rhi/rhi-constants.h"

namespace nitro::rhi::vulkan
{
    VulkanTimer::VulkanTimer(VulkanDevice *device, uint32_t maxTimers)
        : m_device(device), m_maxTimers(maxTimers), m_queryCount(maxTimers * 2)
    {
        m_timestampPeriod = m_device->timestampPeriod;
        m_timestamps.resize(m_queryCount);

        VkQueryPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = m_queryCount;

        for (int i = 0; i < kMaxFramesInFlight; i++)
        {

            checkVkResult(vkCreateQueryPool(m_device->device, &info, nullptr, &m_slots[i].pool),
                          "Failed to create timer query pool");
        }
    }

    VulkanTimer::~VulkanTimer()
    {
        for (auto &slot : m_slots)
        {
            vkDestroyQueryPool(m_device->device, slot.pool, nullptr);
        }
    }

    void VulkanTimer::beginFrame(RHICommandBuffer *cmd)
    {

        m_current = &m_slots[m_device->getCurrentFrameIndex()];
        auto *vkCmd = reinterpret_cast<VulkanCommandBuffer *>(cmd);
        if (m_current->used && m_current->count > 0)
        {
            VkResult r = vkGetQueryPoolResults(m_device->device, m_current->pool, 0, m_current->count,
                                               sizeof(uint64_t) * m_current->count, m_timestamps.data(),
                                               sizeof(uint64_t), VK_QUERY_RESULT_64_BIT); // no WAIT
            if (r == VK_SUCCESS)
                for (auto &[name, idx] : m_current->nameToIndex)
                    m_results[name] = (m_timestamps[idx + 1] - m_timestamps[idx]) * m_timestampPeriod / 1e6f;
        }

        vkCmdResetQueryPool(vkCmd->cmd, m_current->pool, 0, m_queryCount);
        m_current->nameToIndex.clear();
        m_current->count = 0;
        m_current->used = true;
    }

    void VulkanTimer::begin(RHICommandBuffer *cmd, const std::string &name)
    {

        assert(m_current && "beginFrame must be called before begin()");
        auto *vkCmd = reinterpret_cast<VulkanCommandBuffer *>(cmd);

        if (m_current->nameToIndex.find(name) == m_current->nameToIndex.end())
        {

            m_current->nameToIndex[name] = m_current->count;
            m_current->count += 2;
        }

        uint32_t idx = m_current->nameToIndex[name];
        vkCmdWriteTimestamp(vkCmd->cmd,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            m_current->pool, idx);
    }

    void VulkanTimer::end(RHICommandBuffer *cmd, const std::string &name)
    {

        assert(m_current && "beginFrame must be called before end()");

        auto *vkCmd = reinterpret_cast<VulkanCommandBuffer *>(cmd);

        auto it = m_current->nameToIndex.find(name);
        if (it == m_current->nameToIndex.end())
            return;

        uint32_t idx = it->second;
        vkCmdWriteTimestamp(vkCmd->cmd,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            m_current->pool, idx + 1);
    }

    void VulkanTimer::endFrame()
    {
    }

    float VulkanTimer::getResult(const std::string &name) const
    {
        auto it = m_results.find(name);
        return it != m_results.end() ? it->second : 0.0f;
    }
}