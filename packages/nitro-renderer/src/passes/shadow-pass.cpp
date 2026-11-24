#include "nitro-renderer/passes/shadow-pass.h"
#include "nitro-geometry/vertex.h"

namespace
{

    struct ShadowPushConstant
    {
        glm::mat4 viewProj;
    };

}
namespace nitro::renderer
{
    ShadowPassOutputs addShadowPass(RenderContext &ctx, const CullOutputs &cull,
                                    const glm::mat4 &viewProj, uint32_t cascade,
                                    const PipelineSettings::Shadow &settings)
    {
        rhi::TextureDesc shadowDesc;
        shadowDesc.usage = rhi::TextureDesc::Usage::DepthStencil | rhi::TextureDesc::Usage::ShaderRead;
        shadowDesc.format = rhi::TextureDesc::ImageFormat::Depth32Float;

        frame::GPUResourceOptions shadowOptions;

        shadowOptions.scope = frame::ResourceScope::View;
        shadowOptions.viewHandle = ctx.view.handle;
        shadowOptions.lifetime = frame::ResourceLifetimeType::Transient;
        shadowOptions.instanceType = frame::ResourceInstanceType::Single;

        auto shadowHandle = ctx.builder.createTexture("Shadow " + std::to_string(cascade), shadowDesc, frame::ResourceSize::textureAbsolute(settings.resolution, settings.resolution), shadowOptions);

        auto shadowPhysicalHandle = ctx.builder.get(shadowHandle)->physicalHandle;

        std::vector<rhi::RHIDescriptorBinding> bindings{
            {.type = rhi::RHIDescriptorBinding::Type::StorageBuffer,
             .stage = rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             .binding = 0}};
        auto layout = ctx.layouts.get(bindings);

        rhi::PipelineDesc pipelineDesc;

        pipelineDesc.cullMode = rhi::PipelineDesc::CullMode::Back;
        pipelineDesc.hasDepth = true;
        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32Float;
        pipelineDesc.depthWrite = true;
        pipelineDesc.depthTest = rhi::CompareOp::LessOrEqual;
        pipelineDesc.shaders.push_back(ctx.shaders.vertex("shadow-depth"));
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(ShadowPushConstant);
        pipelineDesc.depthBias = {.constant = settings.biasConstant, .slope = settings.biasSlope};
        pipelineDesc.layouts = {layout};
        pipelineDesc.vertexLayout = geometry::Vertex::getPositionOnlyLayout();

        auto *pipeline = ctx.pipelines.get(pipelineDesc);
        auto instancePhysicalHandle = ctx.builder.get(ctx.scene.instances)->physicalHandle;

        auto vertexPhysicalHandle = ctx.builder.get(ctx.scene.vertices)->physicalHandle;
        auto indexPhysicalHandle = ctx.builder.get(ctx.scene.indices)->physicalHandle;

        auto drawCommandPhysicalHandle = ctx.builder.get(cull.drawCommands)->physicalHandle;

        auto maxDrawCount = cull.maxDrawCount;

        const uint32_t res = settings.resolution;
        ShadowPushConstant pc{
            .viewProj = viewProj};
        ctx.builder.addRenderPass("Shadow Pass " + std::to_string(cascade), [=](const frame::PassContext &p)
                                  {
                                      auto *descriptorSet = p.cmd.allocateDescriptorSet(layout);

                                      descriptorSet->writeBuffer(p.resources.get(instancePhysicalHandle, p.frame), 0);
                                      descriptorSet->commit();
                                      rhi::RenderingDesc renderPassDesc;

                                      renderPassDesc.depth.texture = p.resources.get(shadowPhysicalHandle, p.frame);
                                      renderPassDesc.depth.depthLoad = rhi::LoadOp::Clear;
                                      renderPassDesc.depth.depthStore = rhi::StoreOp::Store;
                                      renderPassDesc.depth.clearDepth = 1.0f;


                                      p.cmd.beginRendering(renderPassDesc);
                                      p.cmd.bindPipeline(pipeline);
                                      p.cmd.bindDescriptorSet(descriptorSet, 0);
                                      p.cmd.setPushConstant(&pc, sizeof(ShadowPushConstant), 1);
                                      
                                    rhi::RHIViewport viewport;
                                    viewport.width = res;
                                    viewport.height =  res;

                                    rhi::RHIScissor scissor;
                                    scissor.width =  res;
                                    scissor.height =  res;

                                    p.cmd.setViewPort(viewport);
                                    p.cmd.setScissor(scissor);
                                    
                                      p.cmd.bindVertexBuffer(p.resources.get(vertexPhysicalHandle, p.frame));
                                      p.cmd.bindIndexBuffer(p.resources.get(indexPhysicalHandle, p.frame));

                                      p.cmd.drawIndexedIndirect(
                                        p.resources.get(drawCommandPhysicalHandle,p.frame),
                                        0,
                                        maxDrawCount,
                                        sizeof(rhi::DrawIndexedIndirectArgs)
                                      );
                                      p.cmd.endRendering(); })

            .read(cull.drawCommands, rhi::ResourceState::IndirectDraw)
            .read(ctx.scene.instances, rhi::ResourceState::ShaderRead)
            .read(ctx.scene.vertices, rhi::ResourceState::VertexBuffer)
            .read(ctx.scene.indices, rhi::ResourceState::IndexBuffer)
            .write(shadowHandle, rhi::ResourceState::DepthWrite);

        ShadowPassOutputs output{
            .depth = shadowHandle};

        return output;
    }
} // namespace nitro::renderer
