// vulkan-timer.h
#pragma once
#include <nitro-rhi/rhi-timer.h>
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace nitro::rhi::vulkan
{
    class VulkanDevice;

    class VulkanTimer : public RHITimer
    {
    public:
        VulkanTimer(VulkanDevice *device, uint32_t maxTimers = 16);
        ~VulkanTimer() override;

        void beginFrame(RHICommandBuffer *cmd) override;
        void begin(RHICommandBuffer *cmd, const std::string &name) override;
        void end(RHICommandBuffer *cmd, const std::string &name) override;
        void endFrame() override;
        float getResult(const std::string &name) const override;

    private:
        VulkanDevice *m_device;
        VkQueryPool m_queryPool;
        float m_timestampPeriod;
        uint32_t m_maxTimers;
        uint32_t m_queryCount;

        std::unordered_map<std::string, uint32_t> m_nameToIndex;
        uint32_t m_nextIndex = 0;

        std::vector<uint64_t> m_timestamps;
        std::unordered_map<std::string, float> m_results;
    };
}