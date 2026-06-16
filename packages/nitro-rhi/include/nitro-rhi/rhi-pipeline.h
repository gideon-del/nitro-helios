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
        uint32_t binding;

        struct Attributes
        {
            uint32_t location;
            uint32_t offset;
            enum class Format
            {
                Float,
                Float2,
                Float3,
                Float4
            } format;
        };

        std::vector<Attributes> attributes;

        uint32_t stride;
    };

    enum class ShaderStage
    {
        Fragment,
        Vertex
    };
    struct ShaderDesc
    {
        std::string name;
        std::string filePath;
        ShaderStage stage;
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
            CompareOp compareOp;
            uint32_t compareMask = 0xFF;
            uint32_t writeMask = 0xFF;
            uint32_t reference = 1;
        };
        bool enabled = false;
        StencilFace front;
        StencilFace back;
    };
    struct PipelineDesc
    {
        RHIVertexLayout vertexLayout;
        std::vector<ShaderDesc> shaders;
        bool depthWrite = true;
        CompareOp depthTest = CompareOp::LessOrEqual;
        bool hasColorAttachment = true;
        bool hasDepth = true;

        struct ColorAttachmentDesc
        {
            TextureDesc::ImageFormat format;
            RHIBlendDesc blend;

            ColorAttachmentDesc(TextureDesc::ImageFormat format) : format(format) {}
            ColorAttachmentDesc(TextureDesc::ImageFormat format, RHIBlendDesc blend) : format(format), blend(blend) {}
        };
        std::vector<ColorAttachmentDesc> colorAttachments;
        bool hasPushConstant = true;
        uint32_t pushConstantSize;
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
    };

    class RHIPipeline
    {
    public:
        virtual ~RHIPipeline() = default;
    };
};