#include "nitro-rhi/descriptor-layout-cache.h"

namespace nitro::rhi
{

    DescriptorLayoutCache::~DescriptorLayoutCache()
    {
        for (auto &[key, layout] : m_cache)
            m_device.destroyDescriptorLayout(layout);
    }
    RHIDescriptorLayout *DescriptorLayoutCache::get(std::span<const RHIDescriptorBinding> bindings)
    {
        LayoutBindingKey key{bindings};

        if (auto it = m_cache.find(key);
            it != m_cache.end())
        {
            return it->second;
        }

        auto *layout = m_device.createDescriptorLayout(key.bindings);
        m_cache.emplace(std::move(key), layout);
        return layout;
    }
} // namespace nitro::rhi
