#pragma once
#include "nitro-rhi/rhi-descriptor-layout.h"
#include "nitro-rhi/rhi-device.h"
#include "nitro-core/utils.h"
#include <span>
#include <unordered_map>
#include <algorithm>

namespace nitro::rhi
{

    struct LayoutBindingKey
    {
        std::vector<RHIDescriptorBinding> bindings;

        explicit LayoutBindingKey(
            std::span<const RHIDescriptorBinding> input)
            : bindings(input.begin(), input.end())
        {
            std::sort(
                bindings.begin(),
                bindings.end(),
                [](const auto &a, const auto &b)
                {
                    return a.binding < b.binding;
                });

            auto dup = std::adjacent_find(bindings.begin(), bindings.end(),
                                          [](const auto &a, const auto &b)
                                          { return a.binding == b.binding; });
            assert(dup == bindings.end() && "two descriptors declared for the same binding slot");
        }
        bool operator==(const LayoutBindingKey &o) const = default;
    };

    struct LayoutBindingKeyHash
    {

        size_t operator()(const LayoutBindingKey &key) const
        {
            size_t seed = 0;

            hashCombine(seed, key.bindings.size());
            for (auto &binding : key.bindings)
            {

                hashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(binding.binding)));
                hashCombine(seed, std::hash<int>{}(static_cast<int>(binding.type)));
                hashCombine(seed, std::hash<int>{}(static_cast<int>(binding.stage)));
                hashCombine(seed, std::hash<bool>{}(static_cast<bool>(binding.isBindlessArray)));
                hashCombine(seed, std::hash<uint32_t>{}(static_cast<uint32_t>(binding.bindlessCount)));
            }

            return seed;
        }
    };
    class DescriptorLayoutCache
    {
    public:
        explicit DescriptorLayoutCache(RHIDevice &device) : m_device(device) {};
        ~DescriptorLayoutCache();
        RHIDescriptorLayout *get(std::span<const RHIDescriptorBinding> bindings);

    private:
        RHIDevice &m_device;

        std::unordered_map<
            LayoutBindingKey,
            RHIDescriptorLayout *,
            LayoutBindingKeyHash>
            m_cache;
    };
} // namespace nitro::rhi
