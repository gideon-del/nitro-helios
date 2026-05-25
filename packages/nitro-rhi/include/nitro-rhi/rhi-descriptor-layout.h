#pragma once
#include <cstdint>
namespace nitro::rhi
{
    struct RHIDescriptorBinding
    {
        enum class Type
        {
            UniformBuffer,
            Sampler
        } type;

        enum class ShaderStage
        {
            Vertex,
            Fragment,
            Both
        } stage;

        uint32_t binding;
    };

    class RHIDescriptorLayout
    {
    public:
        virtual ~RHIDescriptorLayout() = default;
    };

} // namespace nitro::rhi
