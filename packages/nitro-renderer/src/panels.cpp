#include <nitro-renderer/panels.h>
#include <imgui.h>

namespace nitro::renderer
{
    void LightPanel::draw(LightingSettings &settings)
    {
        ImGui::Begin("Renderer");

        if (ImGui::CollapsingHeader("Light Camera"))
        {
            ImGui::SliderFloat(
                "Phi",
                &settings.lightCamera.phi,
                0.0f,
                2 * M_PI);

            ImGui::SliderFloat(
                "Theta",
                &settings.lightCamera.theta,
                0.0f,
                M_PI);

            ImGui::SliderFloat(
                "Radius",
                &settings.lightCamera.radius,
                0.0f,
                100.0f);
        }

        if (ImGui::CollapsingHeader("Lighting"))
        {
            // ImGui::SliderFloat(
            //     "Ambient",
            //     &settings.ambient,
            //     0,
            //     1);

            // ImGui::SliderFloat(
            //     "Ka",
            //     &settings.Ka,
            //     0,
            //     1);

            // ImGui::SliderFloat(
            //     "Kd",
            //     &settings.Kd,
            //     0,
            //     1);

            // ImGui::SliderFloat(
            //     "Ks",
            //     &settings.Ks,
            //     0,
            //     1);

            // ImGui::ColorEdit3(
            //     "Light Color",
            //     &settings.lightColor.x);

            ImGui::SliderFloat(
                "Roughness",
                &settings.roughness,
                0,
                1);

            ImGui::SliderFloat(
                "Metallic",
                &settings.metallic,
                0,
                1);
        }

        ImGui::End();
    };

    void ShadowPanel::draw(ShadowSettings &settings)
    {
        ImGui::Begin("Shadows");

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

        ImGui::End();
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

        if (ImGui::Combo(
                "Renderer",
                &currentRenderer,
                renderers,
                IM_ARRAYSIZE(renderers)))
        {
            settings.renderer =
                static_cast<RendererType>(currentRenderer);
        }
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
    }
    void StatPanel::draw(StatSettings &stats)
    {
        if (ImGui::Begin("Frame Stats"))
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
        ImGui::End();
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
        ImGui::Begin("Tonemap");

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
        ImGui::End();
    }

    void BloomPanel::draw(BloomSettings &settings)
    {
        ImGui::Begin("Bloom");
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

        ImGui::End();
    }
    void ColorGradingPanel::draw(ColorGradingSettings &settings)
    {
        ImGui::Begin("Color Grading");
        ImGui::Checkbox("Enable", &settings.enable);
        ImGui::ColorEdit3("Lift", &settings.lift.x);
        ImGui::ColorEdit3("Gain", &settings.gain.x);
        ImGui::ColorEdit3("Gamma", &settings.gamma.x);
        ImGui::End();
    }
    void SSAOPanel::draw(SSAOSettings &settings)
    {
        ImGui::Begin("SSAO");
        ImGui::SliderFloat("Radius", &settings.radius, 0.0f, 3.0f);
        ImGui::SliderFloat("Depth Sigma", &settings.depthSigma, 0.0f, 2.0f);
        ImGui::End();
    }

    void EmitterPanel::draw(ParticleEmitterSystem &system, rhi::RHIBuffer *emitterBuffer)
    {
        ImGui::Begin("Emitters");
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

        ImGui::End();
    }
} // namespace nitro::renderer
