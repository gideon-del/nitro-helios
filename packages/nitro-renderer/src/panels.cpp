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
} // namespace nitro::renderer
