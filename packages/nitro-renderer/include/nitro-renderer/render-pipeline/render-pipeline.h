#pragma once
#include "nitro-renderer/render-context.h"

namespace nitro::renderer
{
    class IRenderPipeline
    {
    public:
        virtual ~IRenderPipeline() = default;
        virtual void declare(RenderContext &ctx) = 0;
    };
} // namespace nitro::renderer
