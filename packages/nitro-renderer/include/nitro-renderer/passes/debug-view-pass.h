#pragma once
#include "nitro-renderer/passes/depth-prepass.h"
#include "nitro-renderer/render-context.h"

namespace nitro::renderer
{
    struct DebugViewOutput
    {
        frame::LogicalTextureHandle output;
    };
    enum class GBufferView : uint32_t
    {
        Depth,
        Albedo,
        Normal,
        Material,
        Emissive,
        WorldPosition,
        Lit
    };
    DebugViewOutput addDebugViewPass(RenderContext &ctx, frame::LogicalTextureHandle source, GBufferView view);

} // namespace nitro::renderer
