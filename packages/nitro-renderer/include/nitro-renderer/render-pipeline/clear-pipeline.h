#pragma once
#include "nitro-renderer/render-pipeline/render-pipeline.h"

namespace nitro::renderer
{
    class ClearPipeline : public IRenderPipeline
    {
    public:
        ~ClearPipeline() override = default;
        void declare(RenderContext &ctx) override;
    };
} // namespace nitro::renderer
