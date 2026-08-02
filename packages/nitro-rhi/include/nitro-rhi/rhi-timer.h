// rhi-timer.h
#pragma once
#include <string>
#include <unordered_map>

namespace nitro::rhi
{

    using TimerResult = std::unordered_map<std::string, float>;
    class RHICommandBuffer;

    class RHITimer
    {
    public:
        virtual ~RHITimer() = default;

        virtual void beginFrame(RHICommandBuffer *cmd) = 0;

        virtual void begin(RHICommandBuffer *cmd, const std::string &name) = 0;
        virtual void end(RHICommandBuffer *cmd, const std::string &name) = 0;

        virtual void endFrame() = 0;

        virtual float getResult(const std::string &name) const = 0;
        virtual TimerResult getAllResults() const = 0;
    };
}