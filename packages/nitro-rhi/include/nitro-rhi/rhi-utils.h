#pragma once

namespace nitro::rhi
{
    enum class ResourceState
    {
        Undefined,

        CopySrc,
        CopyDst,

        ShaderRead,
        ShaderWrite,

        RenderTarget,
        DepthWrite,
        DepthRead,
        DepthReadStencilWrite,

        Present,
    };
    enum class ImageLayout
    {
        Undefined,
        ColorAttachment,
        DepthStencilAttachment,
        DepthAttachmentStencilReadOnly,
        DepthReadOnlyStencilAttachment,
        DepthStencilReadOnly,
        ShaderReadOnly,
        TransferSrc,
        TransferDst,
        Present,
        General
    };
} // namespace nitro::rhi
