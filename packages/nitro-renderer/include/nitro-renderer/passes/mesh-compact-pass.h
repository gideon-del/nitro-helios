#pragma once
#include "nitro-frame/resources/resource-manager.h"
#include "nitro-renderer/render-context.h"
#include <cstdint>

namespace nitro::renderer
{

    struct CullParams
    {
        std::array<glm::vec4, 6> planes;
        bool frustumCull = true;
        bool lodEnabled = true;
        bool occlusion = false;
        bool testNearPlane = true;
        std::string_view name;
    };
    struct CullOutputs
    {
        frame::LogicalBufferHandle drawCommands, drawCount;
        uint32_t maxDrawCount;
    };

    CullOutputs addMeshCompactPass(RenderContext &ctx, const CullParams &params);

} // namespace nitro::renderer
