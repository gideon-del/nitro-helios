#pragma once
#include "nitro-frame/resources/resource-manager.h"
#include "nitro-renderer/render-context.h"
#include <cstdint>

namespace nitro::renderer
{

    struct CullParams
    {
        glm::mat4 view, proj;
        bool frustumCull = true, lod = true;
    };
    struct CullOutputs
    {
        frame::LogicalBufferHandle drawCommands, drawCount;
        uint32_t maxDrawCount;
    };

    CullOutputs addMeshCompactPass(RenderContext &ctx, const CullParams &params);

} // namespace nitro::renderer
