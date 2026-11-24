#pragma once
#include "nitro-renderer/render-context.h"
#include <array>

namespace nitro::renderer
{
    constexpr uint32_t kCascadeCount = 4;

    struct CascadeData
    {
        std::array<glm::mat4, kCascadeCount> viewProj;
        glm::vec4 splits;
    };

    struct ShadowCascadeOutput
    {
        std::array<frame::LogicalTextureHandle, kCascadeCount> outputs;
        CascadeData data;
    };

    CascadeData computeCascades(const CameraData &camera,
                                glm::vec3 lightDir,
                                const PipelineSettings::Shadow &settings);
} // namespace nitro::renderer
