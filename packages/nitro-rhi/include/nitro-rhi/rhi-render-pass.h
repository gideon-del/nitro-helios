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
            LoadOp stencilLoad = LoadOp::Clear;
            StoreOp stencilStore = StoreOp::DontCare;
            float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            float clearDepth = 1.0f;
            uint32_t clearStencil = 0;
            bool depthWrite = true;
            bool stencilWrite = true;
            bool hasStencil = false;
        };

        std::vector<Attachment> colorAttachments;
        Attachment *depthAttachment = nullptr;

        uint32_t width;
        uint32_t height;

        float depthBiasConstant = 0.0f;
        float depthBiasSlopScale = 0.0f;
        float depthBiasClamp = 0.0f;
    };

    class RHIRenderPass
    {
    public:
        virtual ~RHIRenderPass() = default;
    };
}