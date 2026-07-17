#pragma once
#include <nitro-rhi/rhi.h>
#include "vulkan-sampler.h"
#include <unordered_map>

namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanSamplerCache
    {
        std::unordered_map<const RHISamplerDesc, RHISamplerHandle, RHISamplerHash> m_samplerCache;

    public:
        RHISamplerHandle lookup(const RHISamplerDesc &desc)
        {
            auto it = m_samplerCache.find(desc);
            if (it == m_samplerCache.end())
            {
                return RHISamplerHandle{};
            }

            return it->second;
        }

        void add(RHISamplerHandle handle, const RHISamplerDesc &desc)
        {
            m_samplerCache.emplace(desc, handle);
        }
    };
} // namespace nitro::rhi::vulkan
