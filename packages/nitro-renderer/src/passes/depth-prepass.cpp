#include "nitro-renderer/passes/depth-prepass.h"
#include "nitro-geometry/vertex.h"
namespace
{
    struct DepthPrePassCamera
    {
        glm::mat4 view;
        glm::mat4 proj;
    };
}
namespace nitro::renderer
{
    DepthPrepassOutputs addDepthPrepass(RenderContext &ctx, const CullOutputs &cull)
    {

        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             3},

        };

        auto layout = ctx.layouts.get(bindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthTest = rhi::CompareOp::Less;
        pipelineDesc.depthWrite = true;

        pipelineDesc.layouts = {layout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32Float;
        pipelineDesc.hasDepth = true;

        pipelineDesc.hasPushConstant = false;

        pipelineDesc.shaders.push_back(ctx.shaders.vertex("depth-prepass"));

        auto pipeline = ctx.pipelines.get(pipelineDesc);

        rhi::BufferDesc uboDesc;
        uboDesc.size = sizeof(DepthPrePassCamera);
        uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

        frame::GPUResourceOptions uboOptions;
        uboOptions.scope = frame::ResourceScope::View;
        uboOptions.viewHandle = ctx.view.handle;
        uboOptions.instanceType = frame::ResourceInstanceType::PerFrame;
        uboOptions.lifetime = frame::ResourceLifetimeType::Persistent;

        auto uboHandle = ctx.builder.createBuffer("Depth Prepass UBO", uboDesc, frame::ResourceSize::bufferAbsolute(sizeof(DepthPrePassCamera)), uboOptions);

        auto uboPhysicalHandle = ctx.builder.get(uboHandle)->physicalHandle;

        rhi::TextureDesc depthDesc;
        depthDesc.format = rhi::TextureDesc::ImageFormat::Depth32Float;
        depthDesc.usage = rhi::TextureDesc::Usage::DepthStencil | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions depthOptions;
        depthOptions.scope = frame::ResourceScope::View;
        depthOptions.viewHandle = ctx.view.handle;
        depthOptions.instanceType = frame::ResourceInstanceType::Single;
        depthOptions.lifetime = frame::ResourceLifetimeType::Transient;

        auto depthHandle = ctx.builder.createTexture("Depth", depthDesc, frame::ResourceSize::textureViewRelative(), depthOptions);
        auto depthPhysicalHandle = ctx.builder.get(depthHandle)->physicalHandle;

        auto meshInstancePhysicalHandle = ctx.builder.get(ctx.scene.instances)->physicalHandle;

        DepthPrePassCamera camera{
            .view = ctx.request.camera.view,
            .proj = ctx.request.camera.proj,
        };

        auto sceneInstanceCount = cull.maxDrawCount;
        auto vertexPhysicalHandle = ctx.builder.get(ctx.scene.vertices)->physicalHandle;
        auto indexPhysicalHandle = ctx.builder.get(ctx.scene.indices)->physicalHandle;
        auto drawCommandPhysicalHandle = ctx.builder.get(cull.drawCommands)->physicalHandle;
        ctx.builder.addRenderPass("Depth PrePass", [=](const frame::PassContext &p)
                                  {

                                    auto descriptorSet = p.cmd.allocateDescriptorSet(layout);

                                    auto ubo =p.resources.get(uboPhysicalHandle, p.frame);
                                    ubo->upload(&camera, sizeof(DepthPrePassCamera));
                                    descriptorSet->writeBuffer(ubo, 2);
                                    descriptorSet->writeBuffer(p.resources.get(meshInstancePhysicalHandle, p.frame), 3);
                                    descriptorSet->commit();

                                      auto depthTexture = p.resources.get(depthPhysicalHandle, p.frame);
                                      rhi::RenderingDesc renderPassDesc;
                                      renderPassDesc.depth.depthLoad = rhi::LoadOp::Clear;
                                      renderPassDesc.depth.depthStore = rhi::StoreOp::Store;

                                      renderPassDesc.depth.texture = depthTexture;
                                      renderPassDesc.depth.clearDepth = 1.0f;
                                      p.cmd.beginRendering(renderPassDesc);
   
                                     p.cmd.bindPipeline(pipeline);
                                     p.cmd.bindDescriptorSet(descriptorSet, 0);


                                      rhi::RHIViewport viewport;
        viewport.width = p.view.size.width;
        viewport.height =  p.view.size.height;

        rhi::RHIScissor scissor;
        scissor.width =  p.view.size.width;
        scissor.height =  p.view.size.height;

        p.cmd.setViewPort(viewport);
        p.cmd.setScissor(scissor);
        p.cmd.bindVertexBuffer(p.resources.get(vertexPhysicalHandle, p.frame));
        p.cmd.bindIndexBuffer(p.resources.get(indexPhysicalHandle, p.frame));
        p.cmd.drawIndexedIndirect(p.resources.get(drawCommandPhysicalHandle, p.frame),
    0,
sceneInstanceCount, sizeof(rhi::DrawIndexedIndirectArgs));
        
                                      p.cmd.endRendering(); })
            .read(ctx.scene.vertices, rhi::ResourceState::VertexBuffer)
            .read(ctx.scene.indices, rhi::ResourceState::IndexBuffer)
            .read(cull.drawCommands, rhi::ResourceState::IndirectDraw)
            .write(depthHandle, rhi::ResourceState::DepthWrite)
            .read(ctx.scene.instances, rhi::ResourceState::ShaderRead);

        DepthPrepassOutputs output;
        output.depth = depthHandle;

        return output;
    }
} // namespace nitro::renderer
