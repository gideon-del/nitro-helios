#include <nitro-renderer/passes/main-scene-pass.h>
#include <imgui.h>

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
        m_resources.create(g_MAX_FRAMES_IN_FLIGHT,
                           [&](uint32_t frameIdx)
                           {
                               SingleInputPassResource resource;
                               resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                               return resource;
                           });
    }

    MainScenePass::~MainScenePass()
    {
        m_device->destroyPipeline(m_pipeline);
        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void MainScenePass::execute(rhi::RHICommandBuffer *cmd, rhi::RHIRenderPassDesc &desc, RendererSettings &settings, rhi::RHITexture *inputTexture, RenderGraph &renderGraph, ParticleEmitterSystem &system, rhi::RHIBuffer *emitterBuffer,
                                const RenderContext &ctx)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        if (resource.lastInputTexture != inputTexture)
        {

            resource.lastInputTexture = inputTexture;
            rhi::TextureBinding textureBinding;
            textureBinding.texture = inputTexture;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            resource.descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->commit();
        }
        cmd->beginRenderPass(desc);
        // cmd->bindPipeline(m_pipeline);
        // cmd->bindDescriptorSet(resource.descriptorSet, 0);
        // RHIViewScale swapchainViewScale = m_swapchain->getViewScale();
        // rhi::RHIViewport viewport;
        // viewport.width = m_swapchain->getWidth() * swapchainViewScale.x;
        // viewport.height = m_swapchain->getHeight() * swapchainViewScale.y;

        // cmd->setViewPort(viewport);

        // rhi::RHIScissor scissor;
        // scissor.width = m_swapchain->getWidth() * swapchainViewScale.x;
        // scissor.height = m_swapchain->getHeight() * swapchainViewScale.y;
        // cmd->setScissor(scissor);
        // cmd->draw(3);
        m_device->beginImGuiFrame();
        ImGui::DockSpaceOverViewport();
        ImGui::Begin("Settings");
        m_lightPanel.draw(settings.light);
        m_shadowPanel.draw(settings.shadow);
        m_rendererPanel.draw(settings);
        m_statsPanel.draw(settings.stats);
        m_tonemapPanel.draw(settings.tonemap);
        m_bloomPanel.draw(settings.bloom);
        m_colorGradePanel.draw(settings.colorGrading);
        m_ssaoPanel.draw(settings.ssao);
        m_emitterPanel.draw(system, emitterBuffer);

        ImGui::End();
        // renderGraph.drawImGui();
        m_inspectorPanel.draw(ctx);
        m_heirarchyPanel.draw(ctx);
        ImGui::Begin("Viewport");

        ImGuiViewport *vp = ImGui::GetWindowViewport();

        ImVec2 size = ImGui::GetContentRegionAvail();

        size.y *= vp->DpiScale;
        size.x *= vp->DpiScale;

        if (size.x < 100 || size.y < 100)
        {
            size.x = settings.imGuiDockWindow.x;
            size.y = settings.imGuiDockWindow.y;
        }
        else
        {
            settings.imGuiDockWindow = {size.x, size.y};
        }

        bool viewportHovered = ImGui::IsWindowHovered();
        bool viewportFocused = ImGui::IsWindowFocused();

        if (viewportHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;

            ctx.camera->onMouseMove(delta.x, delta.y);
        }

        settings.viewportInputState.focused = viewportFocused;
        settings.viewportInputState.hovered = viewportHovered;
        ImGui::Image((ImTextureID)m_device->getImGuiTextureRef(inputTexture), size);

        ImVec2 vpMin = ImGui::GetItemRectMin();
        ImVec2 vpSize = ImGui::GetItemRectSize();
        if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImVec2 m = ImGui::GetIO().MousePos; // screen space, same as vpMin
            glm::vec2 local = {m.x - vpMin.x,
                               m.y - vpMin.y};
            glm::vec2 uv = local / glm::vec2{vpSize.x, vpSize.y};
            if (uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1)
            {
                geometry::Ray ray = ctx.camera->reconstructRayFromUV(uv);

                ctx.scene->pickMeshInstance(ray);
            }
        }
        auto &selectedInstance = ctx.scene->selectedInstance();
        if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F) && selectedInstance.has_value() && selectedInstance.value().isValid())
        {
            auto instance = ctx.scene->meshManager->getMeshInstance(selectedInstance.value());

            const auto &mn = instance->worldAABBMin;
            const auto &mx = instance->worldAABBMax;
            glm::vec3 size = mx - mn;
            glm::vec3 center = (mn + mx) * 0.5f;

            ctx.camera->focus(center, glm::length(size) * 0.5f);
        }
        ImGui::End();
        m_device->endImGuiFrame();
        m_device->drawImGui(cmd);
        cmd->endRenderPass();
    }

} // namespace nitro::renderer
