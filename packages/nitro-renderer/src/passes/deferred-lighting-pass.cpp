#include "nitro-renderer/passes/deferred-lighting-pass.h"

namespace
{
    using namespace nitro::renderer;
    struct LightingFrame
    {
        glm::mat4 invViewProj;
        glm::vec4 cameraPosition;
        glm::vec4 lightDirection;
        glm::vec4 lightColor;
        glm::vec4 ambient;
        glm::vec4 skyColor;
        uint32_t debugMode;
        uint32_t prefilterMipCount;
        float envIntensity;
        uint32_t hasEnvironment;
    };
    static_assert(sizeof(LightingFrame) == 160);

    struct ShadowFrame
    {
        glm::mat4 cascadeViewProj[4];
        glm::mat4 view;
        glm::vec4 splits;
        glm::vec4 params;
        glm::vec4 texelWorld;
    };

    static_assert(sizeof(ShadowFrame) == 368);

    uint32_t
    toLightingDebug(PipelineSettings::DebugMode mode)
    {
        using M = PipelineSettings::DebugMode;
        switch (mode)
        {
        case M::GGX:
            return 1;
        case M::Fresnel:
            return 2;
        case M::Geometry:
            return 3;
        case M::Ambient:
            return 4;
        case M::Direct:
            return 5;
        case M::DiffuseIBL:
            return 6;
        case M::SpecularIBL:
            return 7;
        case M::Cascade:
            return 8;
        case M::Lit:
        case M::Depth:
        case M::Albedo:
        case M::Normal:
        case M::Material:
        case M::Emissive:
        case M::WorldPosition:
            return 0;
        }
        return 0;
    }
}
namespace nitro::renderer
{
    LightingOutputs addDeferredLightingPass(RenderContext &ctx, const GBufferOutputs &gbuf, const ShadowCascadeOutput &shadow)
    {
        rhi::TextureDesc lightingDesc;
        lightingDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        lightingDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        frame::GPUResourceOptions lightingOptions;
        lightingOptions.scope = frame::ResourceScope::View;
        lightingOptions.viewHandle = ctx.view.handle;
        lightingOptions.lifetime = frame::ResourceLifetimeType::Transient;
        lightingOptions.instanceType = frame::ResourceInstanceType::Single;

        auto lightingHandle = ctx.builder.createTexture("Lighting", lightingDesc, frame::ResourceSize::textureViewRelative(), lightingOptions);
        auto lightingPhysicalHandle = ctx.builder.get(lightingHandle)->physicalHandle;

        auto albedoPhysicalHandle = ctx.builder.get(gbuf.albedo)->physicalHandle;
        auto normalPhysicalHandle = ctx.builder.get(gbuf.normal)->physicalHandle;
        auto materialPhysicalHandle = ctx.builder.get(gbuf.material)->physicalHandle;
        auto emissivePhysicalHandle = ctx.builder.get(gbuf.emissive)->physicalHandle;
        auto depthPhysicalHandle = ctx.builder.get(gbuf.depth)->physicalHandle;

        auto environmentPhysicalHandle = ctx.builder.get(ctx.scene.environment)->physicalHandle;
        auto irradiancePhysicalHandle = ctx.builder.get(ctx.scene.irradiance)->physicalHandle;
        auto prefilterPhysicalHandle = ctx.builder.get(ctx.scene.prefilter)->physicalHandle;
        auto brdfLUTPhysicalHandle = ctx.builder.get(ctx.scene.brdfLUT)->physicalHandle;

        rhi::BufferDesc uboDesc;

        uboDesc.usage = rhi::BufferDesc::Usage::Uniform;
        uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;

        frame::GPUResourceOptions uboOptions;
        uboOptions.scope = frame::ResourceScope::View;
        uboOptions.viewHandle = ctx.view.handle;
        uboOptions.lifetime = frame::ResourceLifetimeType::Persistent;
        uboOptions.instanceType = frame::ResourceInstanceType::PerFrame;

        auto uboHandle = ctx.builder.createBuffer("Lighting UBO", uboDesc, frame::ResourceSize::bufferAbsolute(sizeof(LightingFrame)), uboOptions);
        auto uboPhysicalHandle = ctx.builder.get(uboHandle)->physicalHandle;

        rhi::BufferDesc shadowUBODesc;

        shadowUBODesc.usage = rhi::BufferDesc::Usage::Uniform;
        shadowUBODesc.storage = rhi::BufferDesc::StorageMode::Shared;

        frame::GPUResourceOptions shadowUBOOptions;
        shadowUBOOptions.scope = frame::ResourceScope::View;
        shadowUBOOptions.viewHandle = ctx.view.handle;
        shadowUBOOptions.lifetime = frame::ResourceLifetimeType::Persistent;
        shadowUBOOptions.instanceType = frame::ResourceInstanceType::PerFrame;

        auto shadowUboHandle = ctx.builder.createBuffer("Shadow UBO", shadowUBODesc, frame::ResourceSize::bufferAbsolute(sizeof(ShadowFrame)), shadowUBOOptions);
        auto shadowUboPhysicalHandle = ctx.builder.get(shadowUboHandle)->physicalHandle;

        std::vector<rhi::RHIDescriptorBinding> bindings = {
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             0},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             1},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             3},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             4},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             5},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             6},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             7},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             8},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             9},
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             10},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             11},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             12},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             13},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             14},
        };

        auto layout = ctx.layouts.get(bindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.hasPushConstant = false;
        pipelineDesc.layouts = {layout};
        pipelineDesc.hasDepth = false;
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorRGBA16};

        pipelineDesc.shaders.push_back(ctx.shaders.vertex("lighting"));
        pipelineDesc.shaders.push_back(ctx.shaders.fragment("lighting"));

        auto pipeline = ctx.pipelines.get(pipelineDesc);

        LightingFrame lightingFrame{
            .invViewProj = ctx.request.camera.invViewProj,
            .cameraPosition = glm::vec4(ctx.request.camera.position, 0.0f),
            .lightDirection = glm::vec4(ctx.request.sceneData->directionalLight.direction, 0.0f),
            .lightColor = glm::vec4(ctx.request.sceneData->directionalLight.color, ctx.request.sceneData->directionalLight.intensity),
            .ambient = glm::vec4(ctx.request.sceneData->environment.ambientColor, ctx.request.sceneData->environment.ambientIntensity),
            .skyColor = glm::vec4(ctx.request.sceneData->environment.skyColor, 1.0f),
            .debugMode = toLightingDebug(ctx.request.pipelineSettings.debugMode),
            .prefilterMipCount = ctx.request.sceneData->environment.prefilterMipCount,
            .envIntensity = ctx.request.sceneData->environment.intensity,
            .hasEnvironment = ctx.request.sceneData->environment.valid() ? 1u : 0u};
        auto sampler = ctx.samplers.linearClamp;
        auto shadowSampler = ctx.samplers.shadow;
        std::array<frame::PhysicalTextureHandle, kCascadeCount> shadowDepths;

        for (size_t i = 0; i < shadow.outputs.size(); i++)
        {
            shadowDepths[i] = ctx.builder.get(shadow.outputs[i])->physicalHandle;
        }
        ShadowFrame shadowFrame;

        shadowFrame.params = glm::vec4(ctx.request.pipelineSettings.shadow.normalOffset, 1.0f / float(ctx.request.pipelineSettings.shadow.resolution), ctx.request.pipelineSettings.shadow.enabled ? 1.0f : 0.0f, 0.0f);
        shadowFrame.splits = shadow.data.splits;
        shadowFrame.view = ctx.request.camera.view;
        shadowFrame.texelWorld = shadow.data.texelWorld;
        for (size_t i = 0; i < shadow.data.viewProj.size(); i++)
        {
            shadowFrame.cascadeViewProj[i] = shadow.data.viewProj[i];
        }

        auto &renderPass = ctx.builder.addRenderPass("Deferred Lighting", [=](const frame::PassContext &p)
                                                     {
                                      auto *ubo = p.resources.get(uboPhysicalHandle, p.frame);
                                      ubo->upload(&lightingFrame, sizeof(LightingFrame));

                                      auto *shadowUbo = p.resources.get(shadowUboPhysicalHandle, p.frame);
                                      shadowUbo->upload(&shadowFrame, sizeof(ShadowFrame));

                                      auto *descriptorSet = p.cmd.allocateDescriptorSet(layout);

                                      descriptorSet->writeBuffer(ubo, 0);
                                      rhi::TextureBinding textureBinding;
                                      textureBinding.sampler = sampler;

                                      textureBinding.texture = p.resources.get(albedoPhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 1, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(normalPhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(materialPhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 3, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(emissivePhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 4, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(depthPhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 5, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(environmentPhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 6, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(irradiancePhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 7, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(prefilterPhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 8, rhi::ImageLayout::ShaderReadOnly);

                                      textureBinding.texture = p.resources.get(brdfLUTPhysicalHandle, p.frame);
                                      descriptorSet->writeTexture(textureBinding, 9, rhi::ImageLayout::ShaderReadOnly);

                                         descriptorSet->writeBuffer(shadowUbo, 10);

                                       textureBinding.sampler = shadowSampler;
                                    
                   for(size_t i=0; i < shadowDepths.size(); i++){

                                      textureBinding.texture = p.resources.get(shadowDepths[i], p.frame);
                                      descriptorSet->writeTexture(textureBinding, 11 +i, rhi::ImageLayout::ShaderReadOnly);
                         }


                                   

                                      descriptorSet->commit();

                                      rhi::RenderingDesc renderPassDesc;

                                      renderPassDesc.colors = {{{.texture = p.resources.get(lightingPhysicalHandle, p.frame),
                                                                 .load = rhi::LoadOp::Clear,
                                                                 .store = rhi::StoreOp::Store}}};

                                      p.cmd.beginRendering(renderPassDesc);
                                      p.cmd.bindPipeline(pipeline);
                                      p.cmd.bindDescriptorSet(descriptorSet, 0);

                                      rhi::RHIViewport viewport;
                                      viewport.width = p.view.size.width;
                                      viewport.height = p.view.size.height;

                                      rhi::RHIScissor scissor;
                                      scissor.width = p.view.size.width;
                                      scissor.height = p.view.size.height;

                                      p.cmd.setViewPort(viewport);
                                      p.cmd.setScissor(scissor);

                                      p.cmd.draw(3);
                                      p.cmd.endRendering(); })
                               .read(gbuf.albedo, rhi::ResourceState::ShaderRead)
                               .read(gbuf.normal, rhi::ResourceState::ShaderRead)
                               .read(gbuf.material, rhi::ResourceState::ShaderRead)
                               .read(gbuf.emissive, rhi::ResourceState::ShaderRead)
                               .read(gbuf.depth, rhi::ResourceState::ShaderRead)
                               .write(lightingHandle, rhi::ResourceState::RenderTarget);

        for (auto &s : shadow.outputs)
        {
            renderPass.read(s, rhi::ResourceState::ShaderRead);
        }
        LightingOutputs output{
            .sceneColor = lightingHandle};

        return output;
    }
} // namespace nitro::renderer
