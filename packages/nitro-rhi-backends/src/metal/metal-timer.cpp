#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-timer.h>

namespace nitro::rhi::metal
{
    MetalTimer::MetalTimer(MetalDevice *device) : m_device(device) {}

    void MetalTimer::beginFrame(RHICommandBuffer *cmd)
    {
        m_startTimes.clear();
        m_endTimes.clear();
    }

    void MetalTimer::begin(RHICommandBuffer *cmd, const std::string &name)
    {
        m_startTimes[name] = 0.0;
    }

    void MetalTimer::end(RHICommandBuffer *cmd, const std::string &name)
    {
        auto *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);
        const std::string capName = name;

        metalCmd->commandBuffer->addCompletedHandler(
            [this, capName](MTL::CommandBuffer *buf)
            {
                double start = buf->GPUStartTime();
                double end = buf->GPUEndTime();
                m_results[capName] = float((end - start) * 1000.0);
            });
    }

    void MetalTimer::endFrame()
    {
    }

    float MetalTimer::getResult(const std::string &name) const
    {
        auto it = m_results.find(name);
        return it != m_results.end() ? it->second : 0.0f;
    }
} // namespace nitro::rhi::metal
