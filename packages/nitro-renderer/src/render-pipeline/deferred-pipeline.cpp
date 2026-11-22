#include "nitro-renderer/render-pipeline/deferred-pipeline.h"
#include "nitro-renderer/passes/mesh-compact-pass.h"
#include "nitro-renderer/passes/depth-prepass.h"
#include "nitro-renderer/passes/gbuffer-pass.h"
#include "nitro-renderer/passes/debug-view-pass.h"
#include "nitro-renderer/passes/deferred-lighting-pass.h"
#include "nitro-renderer/passes/tonemap-pass.h"
#include "nitro-renderer/stage-outputs.h"
namespace nitro::renderer
{

    namespace
    {
        using namespace nitro::renderer;

        frame::LogicalTextureHandle pick(const GBufferOutputs &g, PipelineSettings::DebugMode mode)
        {
            switch (mode)
            {
            case PipelineSettings::DebugMode::Albedo:
                return g.albedo;
            case PipelineSettings::DebugMode::Normal:
                return g.normal;
            case PipelineSettings::DebugMode::Material:
                return g.material;
            case PipelineSettings::DebugMode::Emissive:
                return g.emissive;
            case PipelineSettings::DebugMode::Depth:
            case PipelineSettings::DebugMode::WorldPosition:
                return g.depth;
            case PipelineSettings::DebugMode::Lit:
                break;
            }
            assert(false && "mode has no G-buffer source");
            return g.albedo;
        }

        GBufferView toGBufferView(PipelineSettings::DebugMode mode)
        {
            switch (mode)
            {
            case PipelineSettings::DebugMode::Albedo:
                return GBufferView::Albedo;
            case PipelineSettings::DebugMode::Normal:
                return GBufferView::Normal;
            case PipelineSettings::DebugMode::Material:
                return GBufferView::Material;
            case PipelineSettings::DebugMode::Emissive:
                return GBufferView::Emissive;
            case PipelineSettings::DebugMode::Depth:
                return GBufferView::Depth;
            case PipelineSettings::DebugMode::WorldPosition:
                return GBufferView::WorldPosition;
            case PipelineSettings::DebugMode::Lit:
            case PipelineSettings::DebugMode::GGX:
            case PipelineSettings::DebugMode::Fresnel:
            case PipelineSettings::DebugMode::Geometry:
            case PipelineSettings::DebugMode::Ambient:
            case PipelineSettings::DebugMode::Direct:
                break
            }
            assert(false && "mode has no G-buffer view");
            return GBufferView::Albedo;
        }

        void publish(RenderContext &ctx, frame::LogicalTextureHandle output)
        {
            ctx.builder.blackboard().add<SceneColor>().texture = output;
        }

        bool isGBufferView(PipelineSettings::DebugMode m) { return m >= PipelineSettings::DebugMode::Depth && m <= PipelineSettings::DebugMode::WorldPosition; }
    }
    void DeferredPipeline::declare(RenderContext &ctx)
    {
        const auto mode = ctx.request.pipelineSettings.debugMode;
        auto cull = addMeshCompactPass(ctx, {.view = ctx.request.camera.view, .proj = ctx.request.camera.proj});
        auto depthOutput = addDepthPrepass(ctx, cull);

        if (mode == PipelineSettings::DebugMode::Depth)
            return publish(ctx, addDebugViewPass(ctx, depthOutput.depth, GBufferView::Depth).output);
        auto gbuf = addGBufferPass(ctx, cull, depthOutput);
        if (isGBufferView(mode))
            return publish(ctx, addDebugViewPass(ctx, pick(gbuf, mode), toGBufferView(mode)).output);

        auto lightOutput = addDeferredLightingPass(ctx, gbuf);

        publish(ctx, addTonemapPass(ctx, lightOutput.sceneColor).output);
    }
} // namespace nitro::renderer
