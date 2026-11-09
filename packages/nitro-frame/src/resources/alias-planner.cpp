#include "nitro-frame/resources/alias-planner.h"
#include <algorithm>
namespace nitro::frame
{

    static uint64_t handleKey(const PhysicalTextureHandle &handle)
    {

        return (uint64_t(handle.generation) << 32) | handle.index;
    }

    AliasPlan planAliases(std::span<const TransientEntry> entries)
    {

        std::vector<TransientEntry> sorted(entries.begin(), entries.end());
        std::sort(
            sorted.begin(),
            sorted.end(),
            [](const TransientEntry &a, const TransientEntry &b)
            {
                if (a.first != b.first)
                    return a.first < b.first;
                return handleKey(a.handle) < handleKey(b.handle);
            });

        std::vector<AliasGroup> groups;

        for (auto &entry : sorted)
        {

            std::optional<size_t> bestFit;
            size_t bestGrowth = 0;

            for (size_t i = 0; i < groups.size(); i++)
            {
                auto &group = groups[i];

                bool fits = entry.first > group.textures.back().last &&
                            (entry.requirements.memoryTypeBits & group.requirement.memoryTypeBits) != 0;
                if (!fits)
                    continue;

                size_t growth = entry.requirements.size > group.requirement.size
                                    ? entry.requirements.size - group.requirement.size
                                    : 0;

                if (bestFit < 0 ||
                    growth < bestGrowth ||
                    (growth == bestGrowth && group.requirement.size < groups[*bestFit].requirement.size))
                {
                    bestFit = i;
                    bestGrowth = growth;
                }
            };

            if (bestFit)
            {
                auto &group = groups[*bestFit];
                group.requirement.memoryTypeBits &= entry.requirements.memoryTypeBits;
                group.requirement.size = std::max(group.requirement.size, entry.requirements.size);
                group.requirement.alignment = std::max(group.requirement.alignment, entry.requirements.alignment);
                group.textures.push_back(entry);
            }
            else
            {
                HeapId id{handleKey(entry.handle)};

                AliasGroup group;
                group.id = id;
                group.requirement = entry.requirements;
                group.textures.push_back(entry);

                groups.push_back(std::move(group));
            }
        };

        AliasPlan plan;

        for (auto &group : groups)
        {
            for (auto &entry : group.textures)
            {
                plan.textureToHeap[entry.handle] = group.id;
            }
            plan.groups[group.id] = std::move(group);
        }

        return plan;
    }
} // namespace nitro::frame
