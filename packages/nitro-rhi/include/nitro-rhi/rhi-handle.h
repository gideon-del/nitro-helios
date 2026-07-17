#pragma once
#include <cstdint>
namespace nitro::rhi
{
    template <typename T>
    struct RHIHandle
    {
        uint32_t id = 0;
        bool isValid() const { return id != 0; }
        bool operator==(const RHIHandle &o) { return id == o.id; }
    };

    struct SamplerTag
    {
    };

    using RHISamplerHandle = RHIHandle<SamplerTag>;

} // namespace nitro::rhi
