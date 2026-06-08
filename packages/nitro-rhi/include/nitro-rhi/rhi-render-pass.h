#pragma once
#include <vector>
#include "rhi-texture.h"
namespace nitro::rhi
{

    struct RenderPassDesc
    {
        enum class LoadOp
        {
            DontCare,
            Load,
            Clear
        };
        enum class StoreOp
        {
            DontCare,
            Store,
        };
        struct Attachment
        {
            RHITexture *texture;
            LoadOp load = LoadOp::Clear;
            StoreOp store = StoreOp::DontCare;
            float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            float clearDepth = 1.0f;
            bool hasDepth = true;
        };

        std::vector<Attachment> colorAttachments;
        Attachment *depthAttachment = nullptr;

        uint32_t width;
        uint32_t height;
    };

    class RHIRenderPass
    {
    public:
        virtual ~RHIRenderPass() = default;
    };
}