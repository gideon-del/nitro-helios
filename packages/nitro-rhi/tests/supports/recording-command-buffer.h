#pragma once
#include "null-command-buffer.h"

namespace nitro::tests
{

    class RecordingCommandBuffer : public NullCommandBuffer
    {
    public:
        std::vector<ResolvedTextureBarrier> textures;
        std::vector<ResolvedBufferBarrier> buffers;
        int emitCalls = 0;
        const std::vector<ResolvedTextureBarrier> &resolvedTextureBarriers() const { return textures; }
        const std::vector<ResolvedBufferBarrier> &resolvedBufferBarriers() const { return buffers; }
        int barrierEmitCalls() { return emitCalls; }
        void clearRecording()
        {
            emitCalls = 0;
            textures.clear();
            buffers.clear();
        }

    protected:
        void emitBarriers(std::span<const ResolvedTextureBarrier> t,
                          std::span<const ResolvedBufferBarrier> b) override
        {
            ++emitCalls;
            textures.assign(t.begin(), t.end());
            buffers.assign(b.begin(), b.end());
        }
    };
}