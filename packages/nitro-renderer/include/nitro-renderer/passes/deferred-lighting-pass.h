#pragma once
#include "nitro-renderer/render-context.h"
#include "nitro-renderer/passes/gbuffer-pass.h"
#include "nitro-renderer/shadows/cascades.h"

namespace nitro::renderer
{
    struct LightingOutputs
    {
        frame::LogicalTextureHandle sceneColor;
    };
    LightingOutputs addDeferredLightingPass(RenderContext &ctx, const GBufferOutputs &gbuf, const ShadowCascadeOutput &shadow);
} // namespace nitro::renderer
