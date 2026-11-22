#include "nitro-renderer/passes/gbuffer-pass.h"
#include "nitro-geometry/vertex.h"

namespace
{
    struct GeometryCameraBuffer
    {
        glm::mat4 view;
        glm::mat4 proj;
    };
}
namespace nitro::renderer
{
    GBufferOutputs addGBufferPass(RenderContext &ctx, const CullOutputs &cull, const DepthPrepassOutputs &prepass)
    {

        rhi::TextureDesc albedoDesc;
        albedoDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
        albedoDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions albedoOptions;
        albedoOptions.scope = frame::ResourceScope::View;
        albedoOptions.viewHandle = ctx.view.handle;
        albedoOptions.lifetime = frame::ResourceLifetimeType::Transient;
        albedoOptions.instanceType = frame::ResourceInstanceType::Single;

        auto albedoHandle = ctx.builder.createTexture("GBuffer/Albedo", albedoDesc, frame::ResourceSize::textureViewRelative(), albedoOptions);

        auto albedoPhysicalHandle = ctx.builder.get(albedoHandle)->physicalHandle;

        rhi::TextureDesc normalDesc;
        normalDesc.format = rhi::TextureDesc::ImageFormat::ColorRG8U;
        normalDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions normalOptions;
        normalOptions.scope = frame::ResourceScope::View;
        normalOptions.viewHandle = ctx.view.handle;
        normalOptions.lifetime = frame::ResourceLifetimeType::Transient;
        normalOptions.instanceType = frame::ResourceInstanceType::Single;

        auto normalHandle = ctx.builder.createTexture("GBuffer/Normal", normalDesc, frame::ResourceSize::textureViewRelative(), normalOptions);

        auto normalPhysicalHandle = ctx.builder.get(normalHandle)->physicalHandle;

        rhi::TextureDesc materialDesc;
        materialDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
        materialDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions materialOptions;
        materialOptions.scope = frame::ResourceScope::View;
        materialOptions.viewHandle = ctx.view.handle;
        materialOptions.lifetime = frame::ResourceLifetimeType::Transient;
        materialOptions.instanceType = frame::ResourceInstanceType::Single;

        auto materialHandle = ctx.builder.createTexture("GBuffer/Material", materialDesc, frame::ResourceSize::textureViewRelative(), materialOptions);

        auto materialPhysicalHandle = ctx.builder.get(materialHandle)->physicalHandle;

        rhi::TextureDesc emissiveDesc;
        emissiveDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        emissiveDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions emissiveOptions;
        emissiveOptions.scope = frame::ResourceScope::View;
        emissiveOptions.viewHandle = ctx.view.handle;
        emissiveOptions.lifetime = frame::ResourceLifetimeType::Transient;
        emissiveOptions.instanceType = frame::ResourceInstanceType::Single;

        auto emissiveHandle = ctx.builder.createTexture("GBuffer/Emissive", emissiveDesc, frame::ResourceSize::textureViewRelative(), emissiveOptions);

        auto emissivePhysicalHandle = ctx.builder.get(emissiveHandle)->physicalHandle;

        std::vector<rhi::RHIDescriptorBinding> bindings = {
            {rhi::RHIDescriptorBinding::Type::UniformBuffer, rhi::RHIDescriptorBinding::ShaderStage::Vertex, 2},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer, rhi::RHIDescriptorBinding::ShaderStage::Both, 3},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 4},

        };

        auto layout = ctx.layouts.get(bindings);
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorRGBA8, rhi::TextureDesc::ImageFormat::ColorRG8U, rhi::TextureDesc::ImageFormat::ColorRGBA8,
                                         rhi::TextureDesc::ImageFormat::ColorRGBA16};
        pipelineDesc.depthWrite = false;
        pipelineDesc.depthTest = rhi::CompareOp::Equal;

        pipelineDesc.shaders.push_back(ctx.shaders.vertex("gbuffer"));
        pipelineDesc.shaders.push_back(ctx.shaders.fragment("gbuffer"));

        pipelineDesc.layouts = {layout, ctx.request.sceneData->materialTextureLayout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        pipelineDesc.hasPushConstant = false;

        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32Float;
        pipelineDesc.cullMode = PipelineDesc::CullMode::Back;

        auto pipeline = ctx.pipelines.get(pipelineDesc);

        rhi::BufferDesc uboDesc;

        uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

        frame::GPUResourceOptions uboOptions;
        uboOptions.scope = frame::ResourceScope::View;
        uboOptions.viewHandle = ctx.view.handle;
        uboOptions.lifetime = frame::ResourceLifetimeType::Persistent;
        uboOptions.instanceType = frame::ResourceInstanceType::PerFrame;

        auto uboHandle = ctx.builder.createBuffer("GBuffer/UBO", uboDesc, frame::ResourceSize::bufferAbsolute(sizeof(GeometryCameraBuffer)), uboOptions);

        auto uboPhysicalHandle = ctx.builder.get(uboHandle)->physicalHandle;

        GeometryCameraBuffer camera{
            .view = ctx.request.camera.view,
            .proj = ctx.request.camera.proj};

        auto maxDrawCount = cull.maxDrawCount;
        auto meshInstancePhysicalHandle = ctx.builder.get(ctx.scene.instances)->physicalHandle;
        auto materialBufferPhysicalHandle = ctx.builder.get(ctx.scene.materials)->physicalHandle;

        auto materialSet = ctx.request.sceneData->materialTextureSet;
        auto depthPhysicalHandle = ctx.builder.get(prepass.depth)->physicalHandle;
        auto vertexPhysicalHandle = ctx.builder.get(ctx.scene.vertices)->physicalHandle;
        auto indexPhysicalHandle = ctx.builder.get(ctx.scene.indices)->physicalHandle;
        auto drawCommandPhysicalHandle = ctx.builder.get(cull.drawCommands)->physicalHandle;
        ctx.builder.addRenderPass("GBuffer Pass", [=](const frame::PassContext &p)
                                  {
                                    auto* ubo =p.resources.get(uboPhysicalHandle, p.frame);
                                    ubo->upload(&camera, sizeof(GeometryCameraBuffer));

                                      auto *descriptorSet = p.cmd.allocateDescriptorSet(layout);
                                      descriptorSet->writeBuffer(ubo, 2);
                                      descriptorSet->writeBuffer(p.resources.get(meshInstancePhysicalHandle, p.frame), 3);
                                      descriptorSet->writeBuffer(p.resources.get(materialBufferPhysicalHandle, p.frame), 4);
                                      descriptorSet->commit();

                                      rhi::RenderingDesc renderPassDesc;
                                      renderPassDesc.colors = {
                                          {
                                              {.texture = p.resources.get(albedoPhysicalHandle, p.frame),
                                               .load = rhi::LoadOp::Clear,
                                               .store = rhi::StoreOp::Store},
                                              {.texture = p.resources.get(normalPhysicalHandle, p.frame),
                                               .load = rhi::LoadOp::Clear,
                                               .store = rhi::StoreOp::Store},
                                              {.texture = p.resources.get(materialPhysicalHandle, p.frame),
                                               .load = rhi::LoadOp::Clear,
                                               .store = rhi::StoreOp::Store},
                                              {.texture = p.resources.get(emissivePhysicalHandle, p.frame),
                                               .load = rhi::LoadOp::Clear,
                                               .store = rhi::StoreOp::Store},

                                          }};

                                      renderPassDesc.depth.texture = p.resources.get(depthPhysicalHandle, p.frame);

                                      renderPassDesc.depth.depthLoad = rhi::LoadOp::Load;
                                      renderPassDesc.depth.depthStore = rhi::StoreOp::None;
                                      renderPassDesc.depth.readOnly = true;
                                      renderPassDesc.depth.clearDepth = 1.0f;


                                      p.cmd.beginRendering(renderPassDesc);
                                      p.cmd.bindPipeline(pipeline);
                                      p.cmd.bindDescriptorSet(descriptorSet,0);
                                      p.cmd.bindDescriptorSet(materialSet,1);

                                 
                                      
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
    maxDrawCount, 
    sizeof(rhi::DrawIndexedIndirectArgs));




                                      p.cmd.endRendering(); })

            .read(ctx.scene.vertices, ResourceState::VertexBuffer)
            .read(ctx.scene.indices, ResourceState::IndexBuffer)
            .read(cull.drawCommands, ResourceState::IndirectDraw)
            .read(ctx.scene.instances, ResourceState::ShaderRead)
            .read(ctx.scene.materials, ResourceState::ShaderRead)
            .read(prepass.depth, ResourceState::DepthRead)
            .write(albedoHandle, ResourceState::RenderTarget)
            .write(normalHandle, ResourceState::RenderTarget)
            .write(materialHandle, ResourceState::RenderTarget)
            .write(emissiveHandle, ResourceState::RenderTarget);

        GBufferOutputs output{
            .albedo = albedoHandle,
            .normal = normalHandle,
            .material = materialHandle,
            .emissive = emissiveHandle,
            .depth = prepass.depth,
        };

        return output;
    };
} // namespace nitro::renderer
