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
            ImGui::SliderFloat(
                "Ambient",
                &settings.ambient,
                0,
                1);

            ImGui::SliderFloat(
                "Ka",
                &settings.Ka,
                0,
                1);

            ImGui::SliderFloat(
                "Kd",
                &settings.Kd,
                0,
                1);

            ImGui::SliderFloat(
                "Ks",
                &settings.Ks,
                0,
                1);

            ImGui::ColorEdit3(
                "Light Color",
                &settings.lightColor.x);
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
                "Deferred"};
        const char *debugItems[] = {
            "Lit",
            "Albedo",
            "Normal",
            "Depth",
            "World Position",
            "Cascade Colors",
            "Point Light",
            "Directional Light"};
        int currentRenderer =
            static_cast<int>(settings.renderer);
        int currentDebugMode =
            static_cast<int>(settings.selectedDebugMode);

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
} // namespace nitro::renderer
