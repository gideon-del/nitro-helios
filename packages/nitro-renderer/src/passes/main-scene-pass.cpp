#include <nitro-renderer/passes/main-scene-pass.h>

namespace nitro::renderer
{
    MainScenePass::MainScenePass(std::shared_ptr<rhi::RHIDevice> device,
                                 std::shared_ptr<rhi::RHISwapchain> swapchain,
                                 std::string shaderDir,
                                 bool isMetal)
        : m_device(device),
          m_swapchain(swapchain)
    {
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 2}};
        m_descriptorLayout = m_device->createDescriptorLayout(binding);
        std::string shaderPath = shaderDir + "/main-scene-pass/main-scene-pass";
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.hasPushConstant = false;
        pipelineDesc.cullMode = rhi::PipelineDesc::CullMode::None;
        pipelineDesc.layouts = {m_descriptorLayout};
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
    }

    MainScenePass::~MainScenePass()
    {
        m_device->destroyPipeline(m_pipeline);
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void MainScenePass::execute(rhi::RHICommandBuffer *cmd, rhi::RHIRenderPassDesc &desc, RendererSettings &settings, rhi::RHITexture *inputTexture)
    {

        if (m_lastInputTexture != inputTexture)
        {
            m_lastInputTexture = inputTexture;
            m_descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
            rhi::TextureBinding textureBinding;
            textureBinding.texture = m_lastInputTexture;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            m_descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            m_descriptorSet->commit();
        }
        cmd->beginRenderPass(desc);
        cmd->bindPipeline(m_pipeline);
        cmd->bindDescriptorSet(m_descriptorSet, 0);
        RHIViewScale swapchainViewScale = m_swapchain->getViewScale();
        rhi::RHIViewport viewport;
        viewport.width = m_swapchain->getWidth() * swapchainViewScale.x;
        viewport.height = m_swapchain->getHeight() * swapchainViewScale.y;

        cmd->setViewPort(viewport);

        rhi::RHIScissor scissor;
        scissor.width = m_swapchain->getWidth() * swapchainViewScale.x;
        scissor.height = m_swapchain->getHeight() * swapchainViewScale.y;
        cmd->setScissor(scissor);
        cmd->draw(3);

        m_device->beginImGuiFrame();
        m_lightPanel.draw(settings.light);
        m_shadowPanel.draw(settings.shadow);
        m_rendererPanel.draw(settings);
        m_statsPanel.draw(settings.stats);
        m_tonemapPanel.draw(settings.tonemap);
        m_bloomPanel.draw(settings.bloom);
        m_colorGradePanel.draw(settings.colorGrading);
        m_ssaoPanel.draw(settings.ssao);
        m_device->endImGuiFrame();
        m_device->drawImGui(cmd);
        cmd->endRenderPass();
    }
} // namespace nitro::renderer
