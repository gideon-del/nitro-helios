#include <nitro-renderer/passes/cascade-shadow-map-pass.h>
#include <nitro-geometry/vertex.h>

namespace nitro::renderer
{
    CascadeShadowMapPass::CascadeShadowMapPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal) : m_device(device)
    {

        std::vector<rhi::RHIDescriptorBinding> bindings = {
            {rhi ::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             2},
            {rhi ::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             3},
        };
        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthWrite = true;
        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32Float;
        pipelineDesc.hasColorAttachment = false;
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.cullMode = PipelineDesc::CullMode::Front;
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(ShadowPushConstant);
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        pipelineDesc.hasStencil = false;

        std::string shaderPath = shaderDir + "/shadow/shadow";

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs", shaderPath + ".metallib", rhi::ShaderStage::Vertex});
        }
        else
        {
            pipelineDesc.shaders.push_back({"main", shaderPath + ".vert.spv", rhi::ShaderStage::Vertex});
        }

        m_pipeline = m_device->createPipeline(pipelineDesc);

        for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        {
            m_shadowPasses.push_back(ShadowPass(m_device.get(), i));
        }

        rhi::BufferDesc uboDesc;
        uboDesc.size = sizeof(LightView);
        uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frame)
            {
                CascadeShadowMapResource resource;
                rhi::BufferDesc uboDesc;
                uboDesc.size = sizeof(LightView);
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;
                resource.uniformBuffer = m_device->createBuffer(uboDesc);
                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                return resource;
            });
    }

    CascadeShadowMapPass::~CascadeShadowMapPass()
    {
        for (auto &frameResource : m_resources)
        {
            m_device->destroyBuffer(frameResource.uniformBuffer);
        }
        m_device->destroyPipeline(m_pipeline);
    };

    void CascadeShadowMapPass::bindResources(const RGResources &resources, const std::vector<RGTextureID> textures)
    {
        for (int i = 0; i < textures.size(); i++)
        {
            m_shadowPasses[i].bindResource(resources, textures[i]);
        }
    };
    bool CascadeShadowMapPass::isSceneBuffersStale(Scene &scene)
    {
        return m_lastMeshInstanceBuffer != scene.meshManager->instanceBuffer();
    }
    void CascadeShadowMapPass::bindSceneBuffers(Scene &scene)
    {
        m_lastMeshInstanceBuffer = scene.meshManager->instanceBuffer();

        for (auto &resource : m_resources)
        {
            resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
            resource.descriptorSet->writeBuffer(m_lastMeshInstanceBuffer, 3);
            resource.descriptorSet->commit();
        }
    }
    void CascadeShadowMapPass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, CascadeShadowContext ctx, rhi::RHIBuffer *drawCommandsBuffer, rhi::RHIBuffer *drawCountBuffer)
    {
        if (isSceneBuffersStale(scene))
        {
            bindSceneBuffers(scene);
        }

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        LightView lightView;

        for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        {
            lightViewProj[i] = ShadowPass::s_calculateLightOrthoProj(ctx.cameraNear, ctx.cameraFar, CascadeShadowMapPass::CASCADE_COUNT, i,
                                                                     ctx.fov, ctx.aspect, ctx.cameraView, ctx.lightView, ctx.lambda);
            lightView.lightViewProj[i] = lightViewProj[i];
            cascadeSplit[i] = ShadowPass::s_getPracticalSplit(ctx.cameraNear, ctx.cameraFar, CascadeShadowMapPass::CASCADE_COUNT, i, ctx.lambda);
        };

        resource.uniformBuffer->upload(&lightView, sizeof(lightView));

        for (auto &shadowPass : m_shadowPasses)
        {
            shadowPass.execute(cmd, m_pipeline, resource.descriptorSet, scene, drawCommandsBuffer, drawCountBuffer);
        }
    };
} // namespace nitro::renderer
