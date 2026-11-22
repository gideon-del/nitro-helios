#include "nitro-renderer/passes/mesh-compact-pass.h"
#include "nitro-renderer/passes/pass-constants.h"
namespace
{
    struct MeshCompactPushConstant
    {
        glm::mat4 view;
        glm::mat4 proj;
        uint32_t objectCount;
        float projScaleY;
        float screenHeight;
        int32_t maxMip;
        uint32_t lodEnabled, frustumCullEnabled, occlusionCullEnabled;
        float _pad;
    };
    static_assert(sizeof(MeshCompactPushConstant) == 160, "must match the shader's push constant block");

}

namespace nitro::renderer
{
    CullOutputs addMeshCompactPass(RenderContext &ctx, const CullParams &params)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             4},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             5},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             6},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             7},
        };
        auto *layout = ctx.layouts.get(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(MeshCompactPushConstant);

        computePipelineDesc.threadGroupSizeX = 64;
        computePipelineDesc.threadGroupSizeY = 1;
        computePipelineDesc.threadGroupSizeZ = 1;
        computePipelineDesc.layouts = {layout};
        computePipelineDesc.shader = ctx.shaders.compute("mesh-compact");

        auto computePipeline = ctx.pipelines.get(computePipelineDesc);

        rhi::BufferDesc drawCountDesc;
        drawCountDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        drawCountDesc.usage = rhi::BufferDesc::Usage::Indirect | rhi::BufferDesc::Usage::Storage | rhi::BufferDesc::Usage::TransferDst;

        frame::GPUResourceOptions drawCountOptions;
        drawCountOptions.scope = frame::ResourceScope::View;
        drawCountOptions.viewHandle = ctx.view.handle;
        drawCountOptions.instanceType = frame::ResourceInstanceType::PerFrame;
        drawCountOptions.lifetime = frame::ResourceLifetimeType::Persistent;

        auto drawCountHandle = ctx.builder.createBuffer("Geometry Count Buffer", drawCountDesc, frame::ResourceSize::bufferAbsolute(sizeof(uint32_t)), drawCountOptions);

        auto drawCountPhysicalHandle = ctx.builder.get(drawCountHandle)->physicalHandle;
        ctx.builder.addRenderPass("Clear Draw Count", [=](const frame::PassContext &passCtx)
                                  {
        auto drawCountBuffer = passCtx.resources.get(drawCountPhysicalHandle, passCtx.frame);
        passCtx.cmd.fillBuffer(drawCountBuffer, 0, 4, 0); })
            .write(drawCountHandle, rhi::ResourceState::CopyDst);

        assert(ctx.request.sceneData && ctx.request.sceneData->meshInstanceCount <= kMaxDrawCommands && "raise kMaxDrawCommands or add growth");

        rhi::BufferDesc drawCommandDesc;
        drawCommandDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        drawCommandDesc.usage = rhi::BufferDesc::Usage::Indirect | rhi::BufferDesc::Usage::Storage;

        frame::GPUResourceOptions drawCommandOptions;
        drawCommandOptions.scope = frame::ResourceScope::View;
        drawCommandOptions.viewHandle = ctx.view.handle;
        drawCommandOptions.instanceType = frame::ResourceInstanceType::PerFrame;
        drawCommandOptions.lifetime = frame::ResourceLifetimeType::Persistent;

        auto drawCommandHandle = ctx.builder.createBuffer("Geometry Draw Commands", drawCommandDesc, frame::ResourceSize::bufferAbsolute(kMaxDrawCommands * sizeof(rhi::DrawIndexedIndirectArgs)), drawCommandOptions);

        auto drawCommandPhysicalHandle = ctx.builder.get(drawCommandHandle)->physicalHandle;

        rhi::TextureDesc hizDesc;
        hizDesc.format = rhi::TextureDesc::ImageFormat::ColorR32;
        hizDesc.usage = rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions hizOptions;
        hizOptions.scope = frame::ResourceScope::View;
        hizOptions.viewHandle = ctx.view.handle;
        hizOptions.instanceType = frame::ResourceInstanceType::Single;
        hizOptions.historyCount = 2;
        hizOptions.lifetime = frame::ResourceLifetimeType::Persistent;

        auto hizHandle = ctx.builder.createTexture("HiZ", hizDesc, frame::ResourceSize::textureAbsolute(1, 1), hizOptions);

        auto hizPhysicalHandle = ctx.builder.get(hizHandle)->physicalHandle;

        MeshCompactPushConstant pc{};

        pc.objectCount = ctx.request.sceneData->meshInstanceCount;
        pc.frustumCullEnabled = params.frustumCull ? 1 : 0;
        pc.lodEnabled = params.lod ? 1 : 0;
        pc.occlusionCullEnabled = 0;
        pc.proj = ctx.request.camera.proj;
        pc.projScaleY = ctx.request.camera.proj[1][1];
        pc.view = ctx.request.camera.view;
        pc.screenHeight = ctx.view.size.height;

        auto meshesPhysicalHandle = ctx.builder.get(ctx.scene.meshes)->physicalHandle;
        auto instancesPhysicalHandle = ctx.builder.get(ctx.scene.instances)->physicalHandle;
        auto instanceIdsPhysicalHandle = ctx.builder.get(ctx.scene.sceneInstanceIds)->physicalHandle;

        auto sampler = ctx.samplers.linearClamp;
        ctx.builder.addRenderPass("Mesh Compact", [=](const frame::PassContext &p)
                                  {
                                      auto descriptorSet = p.cmd.allocateDescriptorSet(layout);

                                      descriptorSet->writeBuffer(p.resources.get(meshesPhysicalHandle, p.frame), 2);
                                      descriptorSet->writeBuffer(p.resources.get(instancesPhysicalHandle, p.frame), 3);
                                      descriptorSet->writeBuffer(p.resources.get(drawCountPhysicalHandle, p.frame), 4);
                                      descriptorSet->writeBuffer(p.resources.get(drawCommandPhysicalHandle, p.frame), 5);
                                      descriptorSet->writeBuffer(p.resources.get(instanceIdsPhysicalHandle, p.frame), 6);

                                      rhi::TextureBinding binding;
                                      binding.texture = p.resources.get(hizPhysicalHandle, p.frame);
                                      binding.sampler = sampler;

                                      descriptorSet->writeTexture(binding, 7, rhi::ImageLayout::ShaderReadOnly);
                                      descriptorSet->commit();

                                      uint32_t groupSize = std::max((pc.objectCount + 63) / 64, 1u);
                                      p.cmd.bindComputePipeline(computePipeline);
                                      p.cmd.bindComputeDescriptorSet(descriptorSet, 0);
                                    
                                      p.cmd.setPushConstant(&pc, sizeof(MeshCompactPushConstant), 1, true);
                                      p.cmd.dispatch(groupSize, 1, 1); })
            .read(ctx.scene.meshes, rhi::ResourceState::ShaderRead)
            .read(ctx.scene.instances, rhi::ResourceState::ShaderRead)
            .read(ctx.scene.sceneInstanceIds, rhi::ResourceState::ShaderRead)
            .read(hizHandle, rhi::ResourceState::ShaderRead)
            .write(drawCountHandle, rhi::ResourceState::ShaderWrite)
            .write(drawCommandHandle, rhi::ResourceState::ShaderWrite);

        CullOutputs output{
            .drawCommands = drawCommandHandle,
            .drawCount = drawCountHandle};

        output.maxDrawCount = ctx.request.sceneData->meshInstanceCount;

        return output;
    }
} // namespace nitro::renderer
