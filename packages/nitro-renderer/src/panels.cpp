#include <nitro-renderer/panels.h>
#include <imgui.h>

namespace nitro::renderer
{
    void LightPanel::draw(LightingSettings &settings)
    {

        if (ImGui::CollapsingHeader("Light Camera"))
        {
            float phi = settings.lightCamera.phi();
            if (ImGui::SliderFloat("Phi", &phi, 0.0f, glm::two_pi<float>()))
                settings.lightCamera.setPhi(phi);

            float theta = settings.lightCamera.theta();
            if (ImGui::SliderFloat("Theta", &theta, 0.01f, glm::pi<float>() - 0.01f))
                settings.lightCamera.setTheta(theta);

            float radius = settings.lightCamera.radius();
            if (ImGui::SliderFloat("Radius", &radius, 0.1f, 100.0f))
                settings.lightCamera.setRadius(radius);
        }
    };

    void ShadowPanel::draw(ShadowSettings &settings)
    {
        if (ImGui::CollapsingHeader("Shadows"))
        {

            ImGui::SliderFloat(
                "Bias",
                &settings.bias,
                0.0f,
                0.02f);

            ImGui::SliderFloat(
                "Normal Bias",
                &settings.normalBias,
                0.0f,
                0.2f);

            ImGui::SliderFloat(
                "Lambda",
                &settings.lambda,
                0.0f,
                1.0f);

            ImGui::Checkbox(
                "Show Cascade Colors",
                &settings.showCascadeColors);
        }
    };

    void RendererPanel::draw(RendererSettings &settings)
    {
        const char *renderers[] =
            {
                "Forward",
                "Deferred",
                "TiledDeferred"};
        const char *debugItems[] = {
            "Lit",
            "Albedo",
            "Normal",
            "Depth",
            "World Position",
            "Cascade Colors",
            "Point Light",
            "Directional Light",
            "Point Light Heatmap",
            "Distribution Heatmap",
            "Fresnel Schlick",
            "Smith Geometry",
            "Diffuse IBL",
            "Specular IBL",
        };
        const char *lightModeLabels[] = {
            "Blinn Phong",
            "Lambert Diffuse",
            "Cook Torrance"};
        const char *scenes[] = {
            "Main",
            "PBR Grid",
            "Damaged Helmet"};
        int currentRenderer =
            static_cast<int>(settings.renderer);
        int currentDebugMode =
            static_cast<int>(settings.selectedDebugMode);
        int currentLightMode =
            static_cast<int>(settings.selectedLightMode);
        int currentScene =
            static_cast<int>(settings.selectedScene);

        if (ImGui::CollapsingHeader("Renderer"))
        {

            if (ImGui::Combo(
                    "Debug View",
                    &currentDebugMode,
                    debugItems,
                    IM_ARRAYSIZE(debugItems)))
            {
                settings.selectedDebugMode = static_cast<DebugMode>(currentDebugMode);
            }
            if (ImGui::Combo(
                    "Light Mode",
                    &currentLightMode,
                    lightModeLabels,
                    IM_ARRAYSIZE(lightModeLabels)))
            {
                settings.selectedLightMode = static_cast<LightMode>(currentLightMode);
            }
            if (ImGui::Combo(
                    "Scene",
                    &currentScene,
                    scenes,
                    IM_ARRAYSIZE(scenes)))
            {
                settings.selectedScene = static_cast<RendererScenes>(currentScene);
            }

            ImGui::Checkbox("Mesh LOD", &settings.lodEnabled);
            ImGui::Checkbox("Frustum Culling", &settings.frustumCullEnabled);
            ImGui::Checkbox("Occlusion Culling", &settings.occlusionCullEnabled);
            ImGui::Checkbox("Draw Debug Picking", &settings.debugDrawPicking);
            ImGui::Checkbox("Draw Debug Test Boxes", &settings.debugDrawTestedBoxes);
        }
    }
    void StatPanel::draw(StatSettings &stats)
    {
        if (ImGui::CollapsingHeader("Frame Stats"))
        {
            ImGui::Text("FPS: %.1f", stats.fps);
            ImGui::Text("Frame Time: %.2f ms", stats.frameTime);

            ImGui::Separator();

            ImGui::Text("Draw Calls: %u", stats.drawCalls);
            ImGui::Text("Triangles: %u", stats.triangles);
            ImGui::Text("Vertices: %u", stats.vertices);

            ImGui::Separator();

            ImGui::Text("Render Passes");

            for (auto &[passName, frameTime] : stats.perpassTimer)
            {
                if (passName == "frame-time")
                    continue;

                ImGui::Text("%s : %.2f ms", passName.c_str(), frameTime);
            }
        }
    }

    void ToneMapPanel::draw(ToneMapSettings &settings)
    {

        const char *toneMapModes[] =
            {
                "Linear",
                "Reinhard",
                "ACES"};
        int currentMode =
            static_cast<int>(settings.mode);

        if (ImGui::CollapsingHeader("Tonemap"))
        {
            ImGui::Checkbox("Auto Exposure", &settings.autoExposure);

            ImGui::SliderFloat(
                "Exposure",
                &settings.exposure,
                0.0f,
                1.0f);
            if (ImGui::Combo(
                    "Mode",
                    &currentMode,
                    toneMapModes,
                    IM_ARRAYSIZE(toneMapModes)))
            {
                settings.mode = static_cast<ToneMapMode>(currentMode);
            }
        }
    }

    void BloomPanel::draw(BloomSettings &settings)
    {

        if (ImGui::CollapsingHeader("Bloom"))
        {
            ImGui::Checkbox("Enable", &settings.enable);
            ImGui::SliderFloat(
                "Brightness Threshold",
                &settings.threshold,
                0.0f,
                30.0f);
            ImGui::SliderFloat(
                "Intensity",
                &settings.intensity,
                0.0f,
                30.0f);
        }
    }
    void ColorGradingPanel::draw(ColorGradingSettings &settings)
    {

        if (ImGui::CollapsingHeader("Color Grading"))
        {
            ImGui::Checkbox("Enable", &settings.enable);
            ImGui::ColorEdit3("Lift", &settings.lift.x);
            ImGui::ColorEdit3("Gain", &settings.gain.x);
            ImGui::ColorEdit3("Gamma", &settings.gamma.x);
        }
    }
    void SSAOPanel::draw(SSAOSettings &settings)
    {

        if (ImGui::CollapsingHeader("SSAO"))
        {
            ImGui::SliderFloat("Radius", &settings.radius, 0.0f, 3.0f);
            ImGui::SliderFloat("Depth Sigma", &settings.depthSigma, 0.0f, 2.0f);
        }
    }

    void EmitterPanel::draw(ParticleEmitterSystem &system, rhi::RHIBuffer *emitterBuffer)
    {

        if (ImGui::CollapsingHeader("Emitters"))
        {
            const char *emitterTypes[] =
                {
                    "Continuous",
                    "Burst"};
            for (uint32_t i = 0; i < system.getEmitterCount(); i++)
            {
                EmitterDesc &emitter = system.getEmitter(i);
                std::string label = "Emitter " + std::to_string(i);
                ImGui::PushID(i);
                if (ImGui::CollapsingHeader(label.c_str()))
                {

                    if (ImGui::SliderFloat3("Position", &emitter.position.x, -100.0f, 100.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, position),
                                              &emitter.position, sizeof(float) * 3);
                    }
                    if (ImGui::SliderFloat3("Direction", &emitter.direction.x, -1.01f, 1.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, direction),
                                              &emitter.direction, sizeof(float) * 3);
                    }
                    if (ImGui::SliderFloat3("Gravity", &emitter.gravity.x, -30.0f, 30.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, gravity),
                                              &emitter.gravity, sizeof(float) * 3);
                    }
                    if (ImGui::SliderFloat3("Wind", &emitter.wind.x, -30.0f, 30.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, wind),
                                              &emitter.wind, sizeof(float) * 3);
                    }
                    if (ImGui::SliderFloat3("Spawn Area", &emitter.spawnAreaExtent.x, -200.0f, 200.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, spawnAreaExtent),
                                              &emitter.spawnAreaExtent, sizeof(float) * 3);
                    }

                    if (ImGui::ColorEdit4("Start Color", &emitter.startColor.x))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, startColor),
                                              &emitter.startColor, sizeof(glm::vec4));
                    }
                    if (ImGui::ColorEdit4("End Color", &emitter.endColor.x))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, endColor),
                                              &emitter.endColor, sizeof(glm::vec4));
                    }

                    if (ImGui::SliderFloat("Start Size", &emitter.startSize, 0.0001f, 2.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, startSize),
                                              &emitter.startSize, sizeof(float));
                    }
                    if (ImGui::SliderFloat("End Size", &emitter.endSize, 0.01f, 2.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, endSize),
                                              &emitter.endSize, sizeof(float));
                    }

                    if (ImGui::SliderFloat("Sway Amplitude", &emitter.swayAmplitude, 0.0001f, 20.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, swayAmplitude),
                                              &emitter.swayAmplitude, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Sway Frequency", &emitter.swayFrequency, 0.0001f, 20.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, swayFrequency),
                                              &emitter.swayFrequency, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Min Lifetime", &emitter.minLifetime, 0.001f, 10.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, minLifetime),
                                              &emitter.minLifetime, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Max Lifetime", &emitter.maxLifetime, 0.009f, 70.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, maxLifetime),
                                              &emitter.maxLifetime, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Drag", &emitter.drag, 0.0f, 100.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, drag),
                                              &emitter.drag, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Spread", &emitter.spread, 0.01f, glm::pi<float>()))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, spread),
                                              &emitter.spread, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Spawn Rate", &emitter.spawnRate, 10.0f, 10000.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, spawnRate),
                                              &emitter.spawnRate, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Initial Speed", &emitter.initialSpeed, 0.0f, 100.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, initialSpeed),
                                              &emitter.initialSpeed, sizeof(float));
                    }
                    if (ImGui::SliderFloat("Speed Variance", &emitter.speedVariance, 1.0f, 100.0f))
                    {
                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, speedVariance),
                                              &emitter.speedVariance, sizeof(float));
                    }

                    int currentEmitterType = static_cast<int>(emitter.type);

                    if (ImGui::Combo(
                            "Emitter Type",
                            &currentEmitterType,
                            emitterTypes,
                            IM_ARRAYSIZE(emitterTypes)))
                    {
                        emitter.type = static_cast<EmitterType>(currentEmitterType);

                        system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, type),
                                              &emitter.type, sizeof(uint32_t));
                    }
                    if (emitter.type == EmitterType::Burst)
                    {
                        if (ImGui::SliderFloat("Burst Count", &emitter.burstCount, 1.0f, 10000.0f))
                        {
                            system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, burstCount),
                                                  &emitter.burstCount, sizeof(float));
                        }

                        if (ImGui::Button("Explode"))
                        {
                            float resetFire = 0.0f;
                            system.syncFieldToGPU(emitterBuffer, i, offsetof(EmitterDesc, hasFired),
                                                  &resetFire, sizeof(float));
                        }
                    }
                }
                ImGui::PopID();
            }

            if (ImGui::Button("Add Emitter"))
            {
                EmitterDesc newEmitter;
                newEmitter.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                newEmitter.direction = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                newEmitter.startColor = glm::vec4(1.0f, 0.9f, 0.3f, 1.0f);
                newEmitter.endColor = glm::vec4(0.8f, 0.15f, 0.0f, 1.0f);
                newEmitter.startSize = 0.1f;
                newEmitter.endSize = 0.02f;
                newEmitter.spawnRate = 200.0f;
                newEmitter.initialSpeed = 1.0f;
                newEmitter.speedVariance = 0.3f;
                newEmitter.spread = glm::radians(15.0f);
                newEmitter.gravity = glm::vec4(0.0, 9.8f, 0.0f, 0.0f);
                newEmitter.drag = 1.0;
                newEmitter.minLifetime = 0.8f;
                newEmitter.maxLifetime = 1.5f;

                system.addEmitter(newEmitter, emitterBuffer);
            }
        }
    }

    void InspectorPanel::draw(const RenderContext &ctx)
    {
        ImGui::Begin("Inspector");

        auto &selectedInstance = ctx.scene->selectedInstance();

        if (!selectedInstance.has_value() || !selectedInstance.value().isValid())
        {
            ImGui::Text("Select an Object to Inspect");
            ImGui::End();
            return;
        }

        auto instance = ctx.scene->meshManager->getMeshInstance(selectedInstance.value());
        auto mesh = ctx.scene->meshManager->getMesh(instance->mesh);

        if (mesh)
        {
            ImGui::Text("Mesh: %s", mesh->mesh.name.c_str());
        }
        ImGui::Text(" Instance ID %s", std::to_string(selectedInstance.value().id).c_str());

        ImGui::Separator();

        ImGui::Text("Transform");
        ImGui::BeginDisabled();
        auto translation = instance->transformation.baseTranslation();

        ImGui::DragFloat3("Translation", &translation.x, 0.4f);

        auto scale = instance->transformation.baseScale();

        ImGui::DragFloat3("Scale", &scale.x, 0.4f);

        auto rotation = instance->transformation.baseRotationEuler();

        ImGui::DragFloat3("Rotation", &rotation.x, 0.4f);
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::Text("World Bounds");

        const auto &mn = instance->worldAABBMin;
        const auto &mx = instance->worldAABBMax;
        glm::vec3 size = mx - mn;
        glm::vec3 center = (mn + mx) * 0.5f;

        ImGui::Text("Min    %.2f, %.2f, %.2f", mn.x, mn.y, mn.z);
        ImGui::Text("Max    %.2f, %.2f, %.2f", mx.x, mx.y, mx.z);
        ImGui::Text("Center %.2f, %.2f, %.2f", center.x, center.y, center.z);
        ImGui::Text("Size   %.2f, %.2f, %.2f", size.x, size.y, size.z);

        ImGui::Separator();
        ImGui::Text("Spatial Cells (%zu)", instance->cells.size());

        if (ImGui::BeginChild("cells", ImVec2(0, 80), true))
        {
            for (const auto &c : instance->cells)
                ImGui::Text("(%d, %d)", c.x, c.z);
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Text("Material");

        auto *mat = ctx.scene->materialManager->getMaterial(instance->material);
        if (mat)
        {
            ImGui::Text("Albedo    %.2f, %.2f, %.2f, %.2f",
                        mat->parameters.albedo.r, mat->parameters.albedo.g,
                        mat->parameters.albedo.b, mat->parameters.albedo.a);
            ImGui::Text("Metallic  %.3f", mat->parameters.metallic);
            ImGui::Text("Roughness %.3f", mat->parameters.roughness);

            ImGui::Text("Textures");
            ImGui::BulletText("Albedo:    %s", mat->textures.albedo ? "yes" : "—");
            ImGui::BulletText("Normal:    %s", mat->textures.normalMap ? "yes" : "—");
            ImGui::BulletText("MetalRough:%s", mat->textures.metallicRoughness ? "yes" : "—");
            ImGui::BulletText("Occlusion: %s", mat->textures.occlusionMap ? "yes" : "—");
            ImGui::BulletText("Emissive:  %s", mat->textures.emissive ? "yes" : "—");
        }

        if (ImGui::Button("Focus"))
        {
            ctx.camera->focus(center, glm::length(size) * 0.5f);
        }

        ImGui::End();
    }

    void HierarchyPanel::draw(const RenderContext &ctx)
    {
        ImGui::Begin("Hierarchy");

        auto &scene = *ctx.scene;
        const auto &selected = scene.selectedInstance();
        const auto &ids = scene.instanceIds();

        ImGui::Text("%zu instances", ids.size());
        ImGui::Separator();

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(ids.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                auto handle = ids[i];
                auto *instance = scene.meshManager->getMeshInstance(handle);
                if (!instance)
                    continue;
                auto *mesh = scene.meshManager->getMesh(instance->mesh);

                const char *name = (mesh && !mesh->mesh.name.empty())
                                       ? mesh->mesh.name.c_str()
                                       : "(unnamed)";

                bool isSelected = selected.has_value() && selected.value().id == handle.id;

                ImGui::PushID(static_cast<int>(handle.id));
                if (ImGui::Selectable(name, isSelected))
                    scene.setSelectedInstance(handle);

                if (isSelected && ImGui::IsWindowAppearing())
                    ImGui::SetScrollHereY();
                ImGui::PopID();
            }
        }

        ImGui::End();
    }
} // namespace nitro::renderer
