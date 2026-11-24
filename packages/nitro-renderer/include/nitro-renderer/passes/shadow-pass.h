#pragma once
#include "nitro-renderer/passes/mesh-compact-pass.h"
#include "nitro-renderer/render-context.h"

namespace nitro::renderer
{
    struct ShadowPassOutputs
    {
        frame::LogicalTextureHandle depth;
    };

    ShadowPassOutputs addShadowPass(RenderContext &ctx, const CullOutputs &cull,
                                    const glm::mat4 &viewProj, uint32_t cascade,
                                    const PipelineSettings::Shadow &settings);
} // namespace nitro::renderer
