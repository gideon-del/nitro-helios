#pragma once
#include <nitro-rhi/rhi-timer.h>
#include <nitro-rhi/rhi-command-buffer.h>
#include <SingleHeader/MetalCpp.h>
#include <unordered_map>
#include <string>
namespace nitro::rhi::metal
{

    class MetalDevice;
    class MetalTimer : public RHITimer
    {

    public:
        MetalTimer(MetalDevice *device);
        ~MetalTimer() override = default;

        void beginFrame(RHICommandBuffer *cmd) override;
        void begin(RHICommandBuffer *cmd, const std::string &name) override;
        void end(RHICommandBuffer *cmd, const std::string &name) override;
        void endFrame() override;
        float getResult(const std::string &name) const override;

    private:
        MetalDevice *m_device;
        std::unordered_map<std::string, double> m_startTimes;
        std::unordered_map<std::string, double> m_endTimes;
        std::unordered_map<std::string, float> m_results;
    };

} // namespace nitro::rhi::metal
