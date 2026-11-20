#include "nitro-renderer/render-pipeline/clear-pipeline.h"
#include "nitro-renderer/stage-outputs.h"
namespace nitro::renderer
{
    void ClearPipeline::declare(RenderContext &ctx)
    {

        rhi::TextureDesc desc;
        desc.format = rhi::TextureDesc::ImageFormat::ColorSRGB8;
        desc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions options;
        options.scope = frame::ResourceScope::View;
        options.lifetime = frame::ResourceLifetimeType::Persistent;
        options.viewHandle = ctx.view.handle;
        options.instanceType = frame::ResourceInstanceType::Single;

        auto outputHandle = ctx.builder.createTexture("SceneOutput", desc, frame::ResourceSize::textureViewRelative(), options);

        const auto physicalOutputHandle = ctx.builder.get(outputHandle)->physicalHandle;

        const rhi::RHIDescriptorBinding bindings[] = {
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 0},
        };
        auto *layout = ctx.layouts.get(bindings);
        ctx.builder.addRenderPass("Clear Pass", [=](const frame::PassContext &passCtx)
                                  {
                                    auto* set = passCtx.cmd.allocateDescriptorSet(layout);   
    assert(set);
                                      auto *outputTexture = passCtx.resources.get(physicalOutputHandle, passCtx.frame);
                                      rhi::ColorAttachment color{ .texture = outputTexture, .load = rhi::LoadOp::Clear, .clear = {0.9f, 0.3f, 0.2f, 1.0f} };

passCtx.cmd.beginRendering({ .colors = std::span(&color, 1) });
passCtx.cmd.endRendering(); })
            .write(outputHandle, rhi::ResourceState::RenderTarget);

        ctx.builder.blackboard().add<SceneColor>().texture = outputHandle;
    }
} // namespace nitro::renderer
