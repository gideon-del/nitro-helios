#include "nitro-renderer/passes/tonemap-pass.h"

namespace
{

    using namespace nitro::renderer;
    struct TonemapPC
    {
        float exposure = 1.0f;
        uint32_t mode = 0;
        float _pads[2];
    };

    static bool isDataView(PipelineSettings::DebugMode m)
    {
        using M = PipelineSettings::DebugMode;
        switch (m)
        {
        case M::GGX:
        case M::Fresnel:
        case M::Geometry:
            return true;
        case M::Lit:
        case M::Ambient:
        case M::Direct:
        case M::Depth:
        case M::Albedo:
        case M::Normal:
        case M::Material:
        case M::Emissive:
        case M::WorldPosition:
            return false;
        }
        return false;
    }
}
namespace nitro::renderer
{
    TonemapOutputs addTonemapPass(RenderContext &ctx, frame::LogicalTextureHandle sceneColor)
    {
        rhi::TextureDesc tonemapDesc;
        tonemapDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGB8;
        tonemapDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions tonemapOptions;
        tonemapOptions.scope = frame::ResourceScope::View;
        tonemapOptions.viewHandle = ctx.view.handle;
        tonemapOptions.lifetime = frame::ResourceLifetimeType::Persistent;
        tonemapOptions.instanceType = frame::ResourceInstanceType::Single;

        auto tonemapHandle = ctx.builder.createTexture("Tonemap", tonemapDesc, frame::ResourceSize::textureViewRelative(), tonemapOptions);
        auto tonemapPhysicalHandle = ctx.builder.get(tonemapHandle)->physicalHandle;

        auto sceneColorPhysicalHandle = ctx.builder.get(sceneColor)->physicalHandle;

        std::vector<rhi::RHIDescriptorBinding> bindings{
            {.type = rhi::RHIDescriptorBinding::Type::Sampler,
             .stage = rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             .binding = 0}};

        auto layout = ctx.layouts.get(bindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthTest = rhi::CompareOp::Always;
        pipelineDesc.depthWrite = false;
        pipelineDesc.hasDepth = false;
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(TonemapPC);
        pipelineDesc.cullMode = rhi::PipelineDesc::CullMode::None;
        pipelineDesc.layouts = {layout};
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorSRGB8};

        pipelineDesc.shaders.push_back(ctx.shaders.vertex("tonemap"));
        pipelineDesc.shaders.push_back(ctx.shaders.fragment("tonemap"));

        auto pipeline = ctx.pipelines.get(pipelineDesc);

        const bool raw = isDataView(ctx.request.pipelineSettings.debugMode);
        const auto &tm = ctx.request.postSettings.tonemap;
        TonemapPC pc{
            .exposure = raw ? 1.0f : tm.exposure,
            .mode = raw ? 0u : static_cast<uint32_t>(tm.mode),
        };

        auto sampler = ctx.samplers.linearClamp;

        ctx.builder.addRenderPass("Tonemap", [=](const frame::PassContext &p)
                                  {
                                      auto descriptorSet = p.cmd.allocateDescriptorSet(layout);

                                      rhi::TextureBinding binding;
                                      binding.texture = p.resources.get(sceneColorPhysicalHandle, p.frame);
                                      binding.sampler = sampler;
                                      descriptorSet->writeTexture(binding, 0, rhi::ImageLayout::ShaderReadOnly);
                                      descriptorSet->commit();

                                      rhi::RenderingDesc renderPassDesc;
                                      renderPassDesc.colors = {

                                          {

                                              {.texture = p.resources.get(tonemapPhysicalHandle, p.frame),
                                               .load = rhi::LoadOp::Clear,
                                               .store = rhi::StoreOp::Store}}};
                                            
                                            
                                  p.cmd.beginRendering(renderPassDesc);
                                  p.cmd.bindPipeline(pipeline);
                                  p.cmd.bindDescriptorSet(descriptorSet, 0);
                                  p.cmd.setPushConstant(&pc, sizeof(TonemapPC),1);
                                                                  rhi::RHIViewport viewport;
        viewport.width = p.view.size.width;
        viewport.height =  p.view.size.height;

        rhi::RHIScissor scissor;
        scissor.width =  p.view.size.width;
        scissor.height =  p.view.size.height;

        p.cmd.setViewPort(viewport);
        p.cmd.setScissor(scissor);
        p.cmd.draw(3);

                                  p.cmd.endRendering(); })
            .read(sceneColor, rhi::ResourceState::ShaderRead)
            .write(tonemapHandle, rhi::ResourceState::RenderTarget);

        TonemapOutputs output{
            .output = tonemapHandle};

        return output;
    }
} // namespace nitro::renderer
