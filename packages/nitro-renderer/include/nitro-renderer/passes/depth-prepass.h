#pragma once
#include "nitro-renderer/render-context.h"
#include "nitro-renderer/passes/mesh-compact-pass.h"

namespace nitro::renderer
{
    struct DepthPrepassOutputs
    {
        frame::LogicalTextureHandle depth;
    };

    DepthPrepassOutputs addDepthPrepass(RenderContext &ctx, const CullOutputs &cull);
} // namespace nitro::renderer
