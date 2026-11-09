#pragma once
#include "nitro-core/types/size.h"
#include "nitro-core/types/handle.h"

namespace nitro::frame
{
#pragma once

    struct RenderViewTag
    {
    };

    using RenderViewHandle = Handle<RenderViewTag>;
    using RenderViewHandleHash = HandleHash<RenderViewTag>;

    struct RenderView
    {
        Extent2D size;

        uint32_t frameIdx = 0;
        bool historyValid = false;
        size_t compiledStructureHash = 0;
    };

} // namespace nitro::frame
