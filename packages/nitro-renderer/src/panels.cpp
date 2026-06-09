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

} // namespace nitro::renderer
