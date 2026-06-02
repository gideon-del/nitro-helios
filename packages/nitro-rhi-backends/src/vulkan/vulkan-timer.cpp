// vulkan-timer.cpp
#include <nitro-rhi-backends/vulkan/vulkan-timer.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-command-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>

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

        checkVkResult(vkCreateQueryPool(m_device->device, &info, nullptr, &m_queryPool),
                      "Failed to create timer query pool");
    }

    VulkanTimer::~VulkanTimer()
    {
        if (m_queryPool != VK_NULL_HANDLE)
            vkDestroyQueryPool(m_device->device, m_queryPool, nullptr);
    }

    void VulkanTimer::beginFrame(RHICommandBuffer *cmd)
    {
        auto *vkCmd = reinterpret_cast<VulkanCommandBuffer *>(cmd);
        vkCmdResetQueryPool(vkCmd->cmd, m_queryPool, 0, m_queryCount);
        m_nextIndex = 0;
    }

    void VulkanTimer::begin(RHICommandBuffer *cmd, const std::string &name)
    {
        auto *vkCmd = reinterpret_cast<VulkanCommandBuffer *>(cmd);

        if (m_nameToIndex.find(name) == m_nameToIndex.end())
        {
            m_nameToIndex[name] = m_nextIndex;
            m_nextIndex += 2;
        }

        uint32_t idx = m_nameToIndex[name];
        vkCmdWriteTimestamp(vkCmd->cmd,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            m_queryPool, idx);
    }

    void VulkanTimer::end(RHICommandBuffer *cmd, const std::string &name)
    {
        auto *vkCmd = reinterpret_cast<VulkanCommandBuffer *>(cmd);

        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end())
            return;

        uint32_t idx = it->second;
        vkCmdWriteTimestamp(vkCmd->cmd,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            m_queryPool, idx + 1);
    }

    void VulkanTimer::endFrame()
    {
        if (m_nextIndex == 0)
            return;

        vkGetQueryPoolResults(
            m_device->device,
            m_queryPool, 0, m_nextIndex,
            sizeof(uint64_t) * m_nextIndex,
            m_timestamps.data(),
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

        for (auto &[name, idx] : m_nameToIndex)
        {
            uint64_t start = m_timestamps[idx];
            uint64_t end = m_timestamps[idx + 1];
            m_results[name] = (end - start) * m_timestampPeriod / 1e6f;
        }
    }

    float VulkanTimer::getResult(const std::string &name) const
    {
        auto it = m_results.find(name);
        return it != m_results.end() ? it->second : 0.0f;
    }
}