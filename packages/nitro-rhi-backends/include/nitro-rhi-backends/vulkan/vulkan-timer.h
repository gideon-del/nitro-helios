// vulkan-timer.h
#pragma once
#include <nitro-rhi/rhi-timer.h>
#include <nitro-rhi/rhi-constants.h>
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>
namespace nitro::rhi::vulkan
{
    class VulkanDevice;

    struct QueryPoolSlot
    {
        VkQueryPool pool = VK_NULL_HANDLE;
        bool used = false;
        uint32_t count = 0;
        std::unordered_map<std::string, uint32_t> nameToIndex;
    };
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
        TimerResult getAllResults() const override { return m_results; };

    private:
        VulkanDevice *m_device;
        std::array<QueryPoolSlot, kMaxFramesInFlight> m_slots;
        QueryPoolSlot *m_current = nullptr;
        float m_timestampPeriod;
        uint32_t m_maxTimers;
        uint32_t m_queryCount;

        std::vector<uint64_t> m_timestamps;
        TimerResult m_results;
    };
}