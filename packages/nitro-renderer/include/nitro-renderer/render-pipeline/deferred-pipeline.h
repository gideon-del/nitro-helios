#pragma once
#include "nitro-renderer/render-pipeline/render-pipeline.h"

namespace nitro::renderer
{
    class DeferredPipeline : public IRenderPipeline
    {
    public:
        void declare(RenderContext &ctx) override;
    };
} // namespace nitro::renderer
