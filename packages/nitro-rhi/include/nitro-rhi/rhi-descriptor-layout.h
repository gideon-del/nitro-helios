#pragma once
#include <cstdint>
namespace nitro::rhi
{
    struct RHIDescriptorBinding
    {
        enum class Type
        {
            UniformBuffer,
            Sampler,
            StorageBuffer,
            StorageImage,
            SingleSampler,
            Texture
        } type;

        enum class ShaderStage
        {
            Vertex,
            Fragment,
            Compute,
            Both
        } stage;

        uint32_t binding;
        bool isBindlessArray = false;
        uint32_t bindlessCount = 0;
    };

    class RHIDescriptorLayout
    {
    public:
        virtual ~RHIDescriptorLayout() = default;
    };

} // namespace nitro::rhi
