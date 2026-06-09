#include <nitro-renderer/passes/cascade-shadow-map-pass.h>
#include <nitro-geometry/vertex.h>

namespace nitro::renderer
{
    CascadeShadowMapPass::CascadeShadowMapPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
    {

        std::vector<rhi::RHIDescriptorBinding> bindings = {{rhi ::RHIDescriptorBinding::Type::UniformBuffer,
                                                            rhi::RHIDescriptorBinding::ShaderStage::Vertex,
                                                            2}};
        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthTest = true;
        pipelineDesc.hasColorAttachment = false;
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(ShadowPushConstant);
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();

        std::string shaderPath = shaderDir + "/shadow/shadow";

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs", shaderPath + ".metallib", rhi::ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"fs", shaderPath + ".metallib", rhi::ShaderStage::Fragment});
        }
        else
        {
            pipelineDesc.shaders.push_back({"main", shaderPath + ".vert.spv", rhi::ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"main", shaderPath + ".frag.spv", rhi::ShaderStage::Fragment});
        }

        m_pipeline = m_device->createPipeline(pipelineDesc);

        for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        {
            m_shadowPasses.push_back(ShadowPass(m_device.get(), i));
            m_cascades.push_back(m_shadowPasses[i].shadowTexture);
        }

        rhi::BufferDesc uboDesc;
        uboDesc.size = sizeof(LightView);
        uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

        for (int i = 0; i < FrameResource::MAX_FRAME_RESOURCES; i++)
        {
            FrameResource frameResource(m_device.get());

            rhi::RHIBuffer *uboBuffer = m_device->createBuffer(uboDesc);

            frameResource.setBuffer(FrameResourceId::CSMUniformBuffer, uboBuffer);

            rhi::RHIDescriptorSet *descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

            descriptorSet->writeBuffer(uboBuffer, 2);
            descriptorSet->commit();
            frameResource.setDescriptorSet(FrameResourceId::CSMDescriptorSet, descriptorSet);

            m_frameResources.push_back(frameResource);
        }
    }

    CascadeShadowMapPass::~CascadeShadowMapPass()
    {
        for (auto &frameResource : m_frameResources)
        {
            m_device->destroyBuffer(frameResource.getBuffer(FrameResourceId::CSMUniformBuffer));
        }
        m_device->destroyPipeline(m_pipeline);
    };
    void CascadeShadowMapPass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, CascadeShadowContext ctx)
    {

        uint32_t frameIdx = m_device->getCurrentFrameIndex();

        LightView lightView;

        for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        {
            lightViewProj[i] = ShadowPass::s_calculateLightOrthoProj(ctx.cameraNear, ctx.cameraFar, CascadeShadowMapPass::CASCADE_COUNT, i,
                                                                     ctx.fov, ctx.aspect, ctx.cameraView, ctx.lightView, ctx.lambda);
            lightView.lightViewProj[i] = lightViewProj[i];
            cascadeSplit[i] = ShadowPass::s_getPracticalSplit(ctx.cameraNear, ctx.cameraFar, CascadeShadowMapPass::CASCADE_COUNT, i, ctx.lambda);
        };

        m_frameResources[frameIdx].getBuffer(FrameResourceId::CSMUniformBuffer)->upload(&lightView, sizeof(lightView));

        for (auto &shadowPass : m_shadowPasses)
        {
            shadowPass.execute(cmd, m_pipeline, m_frameResources[frameIdx].getDescriptorSet(FrameResourceId::CSMDescriptorSet), scene);
        }
    };
} // namespace nitro::renderer
