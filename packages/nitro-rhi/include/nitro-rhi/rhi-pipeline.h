#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "rhi-descriptor-layout.h"
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

    struct ShaderDesc
    {
        std::string name;
        std::string filePath;
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
        ShaderDesc vertexShader;
        ShaderDesc fragmentShader;
        bool depthTest = true;
        RHIDescriptorLayout *layout = nullptr;
        PipelineTopology topology = PipelineTopology::TriangleList;
    };

    class RHIPipeline
    {
    public:
        virtual ~RHIPipeline() = default;
    };
};