#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "rhi-descriptor-layout.h"
#include "rhi-texture.h"
namespace nitro::rhi
{

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
    struct PipelineDesc
    {
        RHIVertexLayout vertexLayout;
        std::vector<ShaderDesc> shaders;
        bool depthTest = true;
        bool hasColorAttachment = true;
        std::vector<TextureDesc::ImageFormat> colorAttachments;
        bool hasPushConstant = true;
        uint32_t pushConstantSize;
        std::vector<RHIDescriptorLayout *> layouts;
        PipelineTopology topology = PipelineTopology::TriangleList;
    };

    class RHIPipeline
    {
    public:
        virtual ~RHIPipeline() = default;
    };
};