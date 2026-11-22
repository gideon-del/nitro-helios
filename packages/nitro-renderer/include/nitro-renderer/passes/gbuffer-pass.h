#pragma once
#include "nitro-renderer/render-context.h"
#include "nitro-renderer/passes/depth-prepass.h"
#include "nitro-renderer/passes/mesh-compact-pass.h"

namespace nitro::renderer
{
    struct GBufferOutputs
    {
        frame::LogicalTextureHandle albedo;
        frame::LogicalTextureHandle normal;
        frame::LogicalTextureHandle material;
        frame::LogicalTextureHandle emissive;
        frame::LogicalTextureHandle depth;
    };

    GBufferOutputs addGBufferPass(RenderContext &ctx, const CullOutputs &cull, const DepthPrepassOutputs &prepass);
} // namespace nitro::renderer
