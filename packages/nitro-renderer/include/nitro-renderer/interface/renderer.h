#pragma once
#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-renderer/settings.h>
#include <nitro-renderer/context.h>

namespace nitro::renderer
{
    class IRenderer
    {
    public:
        virtual void execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings) = 0;
        virtual void resize(uint32_t width, uint32_t height) = 0;
    };
} // namespace nitro::renderer
