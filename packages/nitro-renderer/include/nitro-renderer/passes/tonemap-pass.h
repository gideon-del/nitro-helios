#pragma once
#include "nitro-renderer/render-context.h"

namespace nitro::renderer
{
    struct TonemapOutputs
    {
        frame::LogicalTextureHandle output;
    };
    TonemapOutputs addTonemapPass(RenderContext &ctx, frame::LogicalTextureHandle sceneColor);
} // namespace nitro::renderer
