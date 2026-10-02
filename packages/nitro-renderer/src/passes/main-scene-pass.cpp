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
        m_inspectorPanel.draw(ctx, m_gizmoOp, m_gizmoMode, m_dragStartTransform, m_device);
        m_heirarchyPanel.draw(ctx, settings);
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

        if (viewportHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing())
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;

            ctx.camera->onMouseMove(delta.x, delta.y);
        }

        settings.viewportInputState.focused = viewportFocused;
        settings.viewportInputState.hovered = viewportHovered;
        ImGui::Image((ImTextureID)m_device->getImGuiTextureRef(inputTexture), size);

        ImVec2 vpMin = ImGui::GetItemRectMin();
        ImVec2 vpSize = ImGui::GetItemRectSize();

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(vpMin.x, vpMin.y, vpSize.x, vpSize.y);

        if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver())
        {
            ImVec2 m = ImGui::GetIO().MousePos;
            glm::vec2 local = {m.x - vpMin.x,
                               m.y - vpMin.y};
            glm::vec2 uv = local / glm::vec2{vpSize.x, vpSize.y};
            if (uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1)
            {
                geometry::Ray ray = ctx.camera->reconstructRayFromUV(uv);

                ctx.scene->pickEntity(ray);
            }
        }
        auto &selectedEntity = ctx.scene->selectedEntity();
        bool usingNow = ImGuizmo::IsUsing();
        if (selectedEntity.has_value() && selectedEntity->isValid())
        {

            auto *entity = ctx.scene->entityStore()->get(selectedEntity.value());
            glm::mat4 model = entity->transformation.getTransform().model;

            glm::mat4 view = ctx.camera->view();
            glm::mat4 proj = ctx.camera->proj();

            glm::mat4 gizmoProj = proj;

            if (ctx.camera->flipY())
                gizmoProj[1][1] *= -1.0f;
            if (ImGuizmo::Manipulate(&view[0][0], &gizmoProj[0][0],
                                     m_gizmoOp, m_gizmoMode, &model[0][0]))
            {
                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(&model[0][0], t, r, s);
                entity->transformation.setTranslation({t[0], t[1], t[2]});
                entity->transformation.setRotationEuler({r[0], r[1], r[2]});
                entity->transformation.setScale({s[0], s[1], s[2]});
                ctx.scene->updateEntity(*selectedEntity);
            }

            if (usingNow && !m_wasUsingGizmo)
                m_dragStartTransform = entity->transformation;

            if (!usingNow && m_wasUsingGizmo) // falling edge
                ctx.scene->pushCommand(
                    std::make_unique<TransformCommand>(m_dragStartTransform,
                                                       entity->transformation,
                                                       *selectedEntity));
        }

        m_wasUsingGizmo = usingNow;

        if (!ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F) && selectedEntity.has_value() && selectedEntity.value().isValid())
        {
            auto entity = ctx.scene->entityStore()->get(*selectedEntity);

            const auto &mn = entity->worldAABBMin;
            const auto &mx = entity->worldAABBMax;
            glm::vec3 size = mx - mn;
            glm::vec3 center = (mn + mx) * 0.5f;

            ctx.camera->focus(center, glm::length(size) * 0.5f);
        }

        if (!ImGui::GetIO().WantTextInput && viewportHovered)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W))
                m_gizmoOp = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E))
                m_gizmoOp = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R))
                m_gizmoOp = ImGuizmo::SCALE;
            if (ImGui::IsKeyPressed(ImGuiKey_X))
                m_gizmoMode = (m_gizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        ImGuiIO &io = ImGui::GetIO();

        if (!io.WantTextInput)
        {
            bool mod = io.KeyMods & ImGuiMod_Ctrl;
            if (mod && ImGui::IsKeyPressed(ImGuiKey_Z, false))
            {
                if (io.KeyShift)
                    ctx.scene->commands().redo();
                else
                    ctx.scene->commands().undo();
            }
            if (mod && ImGui::IsKeyPressed(ImGuiKey_Y, false))
                ctx.scene->commands().redo();
        }

        if (!io.WantTextInput && viewportHovered && selectedEntity.has_value() && selectedEntity->isValid())
        {
            auto entity = ctx.scene->entityStore()->get(*selectedEntity);

            if (entity && (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)))
            {
                ctx.scene->pushCommand(
                    std::make_unique<DeleteEntityCommand>(*selectedEntity, *entity));
            }

            if (entity && (io.KeyMods & ImGuiMod_Shortcut) && ImGui::IsKeyPressed(ImGuiKey_D, false))
            {
                auto copy = *entity;
                glm::mat4 invView = glm::inverse(ctx.camera->view());
                glm::vec3 right = glm::normalize(glm::vec3(invView[0]));
                glm::vec3 extent = entity->worldAABBMax - entity->worldAABBMin;

                copy.transformation.translate(right * glm::length(extent) * 1.1f);
                if (entity->meshInstance)
                {
                    auto instance = ctx.scene->meshManager->getMeshInstance(*entity->meshInstance);
                    ctx.scene->pushCommand(std::make_unique<CreateMeshInstanceCommand>(instance->mesh, instance->material, copy.transformation));
                }
            }
        }

        if ((io.KeyMods & ImGuiMod_Shortcut) && ImGui::IsKeyPressed(ImGuiKey_S, false) && settings.currentScenePath)
        {
            ctx.scene->serialize(*settings.currentScenePath);
        }

        ImGui::End();
        ImGui::SetNextWindowPos(ImVec2(vpMin.x + 10, vpMin.y + 10));
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::Begin("##viewport_stats", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);

        ImGui::Text("Instances: %zu", ctx.scene->meshCount());
        ImGui::Text("Pool slots: %zu", ctx.scene->meshManager->poolCapacity());
        if (selectedEntity)
            ImGui::Text("Selected: %u", selectedEntity->index);
        else
            ImGui::TextUnformatted("Selected: none");
        ImGui::Text("Last pick: %zu tested, %zu hit",
                    ctx.scene->lastPick().tested.size(), ctx.scene->lastPick().hit.size());
        ImGui::Text("Undo: %zu  Redo: %zu", ctx.scene->commands().undoSize(), ctx.scene->commands().redoSize());
        ImGui::End();
        m_device->endImGuiFrame();
        m_device->drawImGui(cmd);
        cmd->endRenderPass();
    }

} // namespace nitro::renderer
