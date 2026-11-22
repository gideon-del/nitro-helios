#include "nitro-renderer/passes/debug-view-pass.h"

namespace
{
    struct DepthDebugPushConstant
    {
        glm::mat4 invViewProj;
        uint32_t mode;
        float near = 0;
        float far = 1;
    };
}
namespace nitro::renderer
{
    DebugViewOutput addDebugViewPass(RenderContext &ctx, frame::LogicalTextureHandle source, GBufferView view)
    {
        rhi::TextureDesc outputDesc;
        outputDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
        outputDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions outputOptions;
        outputOptions.scope = frame::ResourceScope::View;
        outputOptions.viewHandle = ctx.view.handle;
        outputOptions.lifetime = frame::ResourceLifetimeType::Persistent;
        outputOptions.instanceType = frame::ResourceInstanceType::Single;

        auto outputHandle = ctx.builder.createTexture("Depth Debug", outputDesc, frame::ResourceSize::textureViewRelative(), outputOptions);

        auto outputPhysicalHandle = ctx.builder.get(outputHandle)->physicalHandle;

        auto sourcePhysicalHandle = ctx.builder.get(source)->physicalHandle;

        std::vector<rhi::RHIDescriptorBinding> bindings{{.type = rhi::RHIDescriptorBinding::Type::Sampler,
                                                         .stage = rhi::RHIDescriptorBinding::ShaderStage::Fragment,
                                                         .binding = 0}};

        auto layout = ctx.layouts.get(bindings);

        rhi::PipelineDesc pipelineDesc{};
        pipelineDesc.colorAttachments = {{rhi::TextureDesc::ImageFormat::ColorRGBA8}};
        pipelineDesc.hasDepth = false;
        pipelineDesc.shaders.push_back(ctx.shaders.vertex("debug-view"));
        pipelineDesc.shaders.push_back(ctx.shaders.fragment("debug-view"));
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(DepthDebugPushConstant);
        pipelineDesc.layouts = {layout};

        auto pipeline = ctx.pipelines.get(pipelineDesc);
        auto sampler = ctx.samplers.nearestClamp;

        DepthDebugPushConstant pc{
            .invViewProj = ctx.request.camera.invViewProj,
            .mode = static_cast<uint32_t>(view),
            .near = ctx.request.camera.nearPlane,
            .far = ctx.request.camera.farPlane};
        ctx.builder.addRenderPass("Debug View", [=](const frame::PassContext &p)
                                  {
        auto descriptorSet = p.cmd.allocateDescriptorSet(layout);
         rhi::TextureBinding binding;
         binding.texture = p.resources.get(sourcePhysicalHandle, p.frame);
         binding.sampler = sampler;

         descriptorSet->writeTexture(binding, 0, rhi::ImageLayout::ShaderReadOnly);
         descriptorSet->commit();
        
         rhi::RenderingDesc renderPassDesc;
         rhi::ColorAttachment coloAttachment;
         coloAttachment.texture = p.resources.get(outputPhysicalHandle, p.frame);
         coloAttachment.load = rhi::LoadOp::DontCare;
         coloAttachment.store = rhi::StoreOp::Store;

         renderPassDesc.colors = {{coloAttachment}};

         p.cmd.beginRendering(renderPassDesc);

         p.cmd.bindPipeline(pipeline);
         p.cmd.bindDescriptorSet(descriptorSet,0);
        rhi::RHIViewport viewport;
        viewport.width = p.view.size.width;
        viewport.height =  p.view.size.height;

        rhi::RHIScissor scissor;
        scissor.width =  p.view.size.width;
        scissor.height =  p.view.size.height;

        p.cmd.setViewPort(viewport);
        p.cmd.setScissor(scissor);
         
        p.cmd.setPushConstant(&pc, sizeof(DepthDebugPushConstant), 1);

        p.cmd.draw(3);

         p.cmd.endRendering(); })
            .read(source, rhi::ResourceState::ShaderRead)
            .write(outputHandle, rhi::ResourceState::RenderTarget);

        DebugViewOutput debugOutput{.output = outputHandle};

        return debugOutput;
    };
} // namespace nitro::renderer
