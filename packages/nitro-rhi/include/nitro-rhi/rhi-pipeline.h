#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "rhi-descriptor-layout.h"
#include "rhi-texture.h"
namespace nitro::rhi
{

    enum class CompareOp
    {
        None,
        Less,
        Greater,
        Equal,
        LessOrEqual,
        GreaterOrEqual,
        NotEqual,
        Always
    };

    struct RHIVertexLayout
    {
        uint32_t binding = 0;

        struct Attributes
        {
            uint32_t location = 0;
            uint32_t offset = 0;
            enum class Format
            {
                Float,
                Float2,
                Float3,
                Float4
            } format;

            bool operator==(const Attributes &) const = default;
        };

        std::vector<Attributes> attributes;

        uint32_t stride = 0;

        bool operator==(const RHIVertexLayout &o) const = default;
    };

    enum class ShaderStage
    {
        Fragment,
        Vertex,
        Compute
    };
    struct ShaderDesc
    {
        std::string name;
        std::string filePath;
        ShaderStage stage;

        bool operator==(const ShaderDesc &o) const = default;
    };

    enum class PipelineTopology
    {
        TriangleList,
        LineList,
        PointList
    };
    struct RHIBlendDesc
    {
        bool enabled = false;
        enum class BlendOp
        {
            Add,
            Substract
        } operation = BlendOp::Add;

        enum class BlendFactor
        {
            One,
            Zero
        };
        BlendFactor srcBlendFactor = BlendFactor::One;
        BlendFactor dstBlendFactor = BlendFactor::One;

        bool operator==(const RHIBlendDesc &o) const = default;
    };

    struct RHIStencilDesc
    {

        enum class StencilOp
        {
            INCREMENT,
            DECREMENT,
            KEEP,
            REPLACE,
            ZERO
        };

        struct StencilFace
        {
            StencilOp failOp = StencilOp::KEEP;
            StencilOp passOp = StencilOp::KEEP;
            StencilOp depthFailOp = StencilOp::KEEP;
            CompareOp compareOp = CompareOp::Always;
            uint32_t compareMask = 0xFF;
            uint32_t writeMask = 0xFF;
            uint32_t reference = 1;

            bool operator==(const StencilFace &) const = default;
        };
        bool enabled = false;
        StencilFace front;
        StencilFace back;

        bool operator==(const RHIStencilDesc &o) const = default;
    };
    struct PipelineDesc
    {
        RHIVertexLayout vertexLayout;
        std::vector<ShaderDesc> shaders;
        bool depthWrite = true;
        CompareOp depthTest = CompareOp::LessOrEqual;
        bool hasDepth = true;

        struct ColorAttachmentDesc
        {
            TextureDesc::ImageFormat format;
            RHIBlendDesc blend;

            ColorAttachmentDesc(TextureDesc::ImageFormat format) : format(format) {}
            ColorAttachmentDesc(TextureDesc::ImageFormat format, RHIBlendDesc blend) : format(format), blend(blend) {}

            bool operator==(const ColorAttachmentDesc &o) const = default;
        };
        std::vector<ColorAttachmentDesc> colorAttachments;
        TextureDesc::ImageFormat depthAttachmentFormat = TextureDesc::ImageFormat::Depth32Float;
        bool hasPushConstant = true;
        uint32_t pushConstantSize = 0;
        std::vector<RHIDescriptorLayout *> layouts;
        PipelineTopology topology = PipelineTopology::TriangleList;

        enum class CullMode
        {
            Back,
            Front,
            None
        };

        enum class FrontFace
        {
            ClockWise,
            CounterClockwise
        } frontFace = FrontFace::CounterClockwise;
        CullMode cullMode = CullMode::Back;
        RHIStencilDesc stencil;

        bool operator==(const PipelineDesc &) const = default;
    };

    struct PipelineDescHash
    {
        size_t operator()(const PipelineDesc &desc) const
        {
            size_t seed = 0;

            hashCombine(seed, std::hash<uint32_t>()(desc.vertexLayout.binding));
            hashCombine(seed, std::hash<uint32_t>()(desc.vertexLayout.stride));
            hashCombine(seed, desc.vertexLayout.attributes.size());

            for (auto &attribute : desc.vertexLayout.attributes)
            {
                hashCombine(seed, std::hash<uint32_t>()(attribute.location));
                hashCombine(seed, std::hash<uint32_t>()(attribute.offset));
                hashCombine(seed, std::hash<uint32_t>()(static_cast<uint32_t>(attribute.format)));
            }

            hashCombine(seed, desc.shaders.size());

            for (auto &shader : desc.shaders)
            {
                hashCombine(seed, std::hash<int>{}(static_cast<int>(shader.stage)));
                hashCombine(seed, std::hash<std::string>{}(shader.filePath));
                hashCombine(seed, std::hash<std::string>{}(shader.name));
            }

            hashCombine(seed, std::hash<bool>{}(desc.depthWrite));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.depthTest)));
            hashCombine(seed, std::hash<bool>{}(desc.hasDepth));

            hashCombine(seed, desc.colorAttachments.size());

            for (auto &attachment : desc.colorAttachments)
            {
                hashCombine(seed, std::hash<int>{}(static_cast<int>(attachment.format)));
                hashCombine(seed, std::hash<bool>{}(attachment.blend.enabled));
                hashCombine(seed, std::hash<int>{}(static_cast<int>(attachment.blend.operation)));
                hashCombine(seed, std::hash<int>{}(static_cast<int>(attachment.blend.srcBlendFactor)));
                hashCombine(seed, std::hash<int>{}(static_cast<int>(attachment.blend.dstBlendFactor)));
            }

            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.depthAttachmentFormat)));
            hashCombine(seed, std::hash<bool>{}(desc.hasPushConstant));
            hashCombine(seed, std::hash<uint32_t>{}(desc.pushConstantSize));

            hashCombine(seed, desc.layouts.size());

            for (auto layout : desc.layouts)
            {
                hashCombine(seed, std::hash<RHIDescriptorLayout *>{}(layout));
            }

            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.topology)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.frontFace)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.cullMode)));

            hashCombine(seed, std::hash<bool>{}(desc.stencil.enabled));

            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.front.failOp)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.front.passOp)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.front.depthFailOp)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.front.compareOp)));
            hashCombine(seed, std::hash<uint32_t>{}(desc.stencil.front.compareMask));
            hashCombine(seed, std::hash<uint32_t>{}(desc.stencil.front.writeMask));

            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.back.failOp)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.back.passOp)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.back.depthFailOp)));
            hashCombine(seed, std::hash<int>{}(static_cast<int>(desc.stencil.back.compareOp)));
            hashCombine(seed, std::hash<uint32_t>{}(desc.stencil.back.compareMask));
            hashCombine(seed, std::hash<uint32_t>{}(desc.stencil.back.writeMask));

            return seed;
        }
    };
    class RHIPipeline
    {
    public:
        virtual ~RHIPipeline() = default;
    };
};