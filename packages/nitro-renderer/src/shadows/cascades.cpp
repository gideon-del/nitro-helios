#include "nitro-renderer/shadows/cascades.h"

#include <glm/gtc/matrix_transform.hpp>
namespace
{

    static float logarithmSplit(
        float nearPlane,
        float farPlane,
        uint32_t cascadeCount,
        uint32_t cascadeIndex)
    {
        float ratio = farPlane / nearPlane;

        return nearPlane *
               std::pow(
                   ratio,
                   float(cascadeIndex) /
                       float(cascadeCount));
    }

    static float uniformSplit(
        float nearPlane,
        float farPlane,
        uint32_t cascadeCount,
        uint32_t cascadeIndex)
    {
        return nearPlane +
               (farPlane - nearPlane) *
                   (float(cascadeIndex) / float(cascadeCount));
    }

    static float practicalSplit(
        float nearPlane,
        float farPlane,
        uint32_t cascadeCount,
        uint32_t cascadeIndex,
        float lambda)
    {
        return lambda * logarithmSplit(nearPlane, farPlane, cascadeCount, cascadeIndex) +
               (1.0f - lambda) * uniformSplit(nearPlane, farPlane, cascadeCount, cascadeIndex);
    }
}
namespace nitro::renderer
{
    CascadeData computeCascades(const CameraData &camera,
                                glm::vec3 lightDir,
                                const PipelineSettings::Shadow &settings)
    {

        std::array<float, kCascadeCount + 1> bounds;

        for (uint32_t i = 0; i <= kCascadeCount; ++i)
            bounds[i] = practicalSplit(camera.nearPlane, camera.farPlane, kCascadeCount, i, settings.lambda);
        bounds[0] = camera.nearPlane;
        bounds[kCascadeCount] = camera.farPlane;

        glm::vec3 dir = glm::normalize(lightDir);
        glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
        glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), dir, up);

        CascadeData result;

        glm::mat4 invView = glm::inverse(camera.view);

        auto tanHalfFov = std::tan(camera.fovY / 2.0f);
        for (uint32_t i = 0; i < kCascadeCount; i++)
        {
            const float sliceNear = bounds[i];
            const float sliceFar = bounds[i + 1];
            result.splits[i] = sliceFar;

            float nearHalfHeight = tanHalfFov * sliceNear;
            float nearHalfWidth = camera.aspect * nearHalfHeight;

            float farHalfHeight = tanHalfFov * sliceFar;
            float farHalfWidth = camera.aspect * farHalfHeight;

            std::array<glm::vec3, 8> corners;

            for (int x = 0; x < 2; x++)
            {
                float xSide = x == 0 ? -1 : 1;
                for (int y = 0; y < 2; y++)
                {
                    float ySide = y == 0 ? -1 : 1;

                    glm::vec4 cornerF = invView * glm::vec4(xSide * farHalfWidth, ySide * farHalfHeight, -sliceFar, 1.0f);

                    glm::vec4 cornerN = invView * glm::vec4(xSide * nearHalfWidth, ySide * nearHalfHeight, -sliceNear, 1.0f);
                    auto cell = (x * 2 + y) * 2;
                    corners[cell] = glm::vec3((lightView * cornerN));
                    corners[cell + 1] = glm::vec3(lightView * cornerF);
                };
            }

            glm::vec3 centre(0.0f);
            for (const auto &c : corners)
                centre += glm::vec3(c);
            centre /= float(corners.size());

            float radius = 0.0f;
            for (const auto &c : corners)
                radius = std::max(radius, glm::length(glm::vec3(c) - centre));
            radius = std::ceil(radius * 16.0f) / 16.0f;

            const float texel = (2.0f * radius) / float(settings.resolution);
            centre.x = std::floor(centre.x / texel) * texel;
            centre.y = std::floor(centre.y / texel) * texel;

            const float zNear = -(centre.z + radius + settings.casterMargin);
            const float zFar = -(centre.z - radius);
            const glm::mat4 proj = glm::orthoRH_ZO(centre.x - radius, centre.x + radius,
                                                   centre.y - radius, centre.y + radius,
                                                   zNear, zFar);
            result.viewProj[i] = proj * lightView;
        }

        return result;
    }
} // namespace nitro::renderer
