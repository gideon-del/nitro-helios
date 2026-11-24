#include "nitro-renderer/render-pipeline/deferred-pipeline.h"
#include "nitro-renderer/passes/mesh-compact-pass.h"
#include "nitro-renderer/passes/depth-prepass.h"
#include "nitro-renderer/passes/gbuffer-pass.h"
#include "nitro-renderer/passes/debug-view-pass.h"
#include "nitro-renderer/passes/deferred-lighting-pass.h"
#include "nitro-renderer/passes/tonemap-pass.h"
#include "nitro-renderer/stage-outputs.h"
#include <array>
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
                break;
            }
            assert(false && "mode has no G-buffer view");
            return GBufferView::Albedo;
        }

        void publish(RenderContext &ctx, frame::LogicalTextureHandle output)
        {
            ctx.builder.blackboard().add<SceneColor>().texture = output;
        }

        bool isGBufferView(PipelineSettings::DebugMode m) { return m >= PipelineSettings::DebugMode::Depth && m <= PipelineSettings::DebugMode::WorldPosition; }
        std::array<glm::vec4, 6> extractFrustum(const glm::mat4 &vp)
        {
            std::array<glm::vec4, 6> planes;
            // Left Plane
            planes[0] = glm::vec4(
                vp[0][3] + vp[0][0],
                vp[1][3] + vp[1][0],
                vp[2][3] + vp[2][0],
                vp[3][3] + vp[3][0]);

            // Right Plane
            planes[1] = glm::vec4(
                vp[0][3] - vp[0][0],
                vp[1][3] - vp[1][0],
                vp[2][3] - vp[2][0],
                vp[3][3] - vp[3][0]);
            // Top Plane
            planes[2] = glm::vec4(
                vp[0][3] + vp[0][1],
                vp[1][3] + vp[1][1],
                vp[2][3] + vp[2][1],
                vp[3][3] + vp[3][1]);

            // Bottom Plane
            planes[3] = glm::vec4(
                vp[0][3] - vp[0][1],
                vp[1][3] - vp[1][1],
                vp[2][3] - vp[2][1],
                vp[3][3] - vp[3][1]);

            // Near Plane
            planes[4] = glm::vec4(
                vp[0][2],
                vp[1][2],
                vp[2][2],
                vp[3][2]);

            // Far Plane
            planes[5] = glm::vec4(
                vp[0][3] - vp[0][2],
                vp[1][3] - vp[1][2],
                vp[2][3] - vp[2][2],
                vp[3][3] - vp[3][2]);

            return planes;
        };
    }

    void DeferredPipeline::declare(RenderContext &ctx)
    {
        const auto mode = ctx.request.pipelineSettings.debugMode;
        auto cull = addMeshCompactPass(ctx, {.planes = extractFrustum(ctx.request.camera.viewProj),
                                             .frustumCull = true,
                                             .lodEnabled = true,
                                             .occlusion = false,
                                             .testNearPlane = true,
                                             .name = "Geometry"});
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
