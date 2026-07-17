#pragma once
#include <vector>
#include <cstdint>
#include <assert.h>
namespace nitro::rhi
{

    using RHIPoolID = uint32_t;
    template <typename T>
    class RHIPool
    {
        std::vector<T> m_slots;
        std::vector<RHIPoolID> m_freeSlot;

    public:
        RHIPoolID allocate(T &&resource)
        {
            if (!m_freeSlot.empty())
            {
                auto id = m_freeSlot.back();

                m_slots[id - 1] = std::move(resource);
                m_freeSlot.pop_back();
                return id;
            }

            RHIPoolID id = static_cast<RHIPoolID>(m_slots.size()) + 1;
            m_slots.push_back(std::move(resource));
            return id;
        }

        T &get(RHIPoolID id)
        {

            assert(id > 0);
            assert(id <= m_slots.size());
            return m_slots[id - 1];
        }

        void free(RHIPoolID id)
        {
            assert(id > 0);
            assert(id <= m_slots.size());

            m_freeSlot.push_back(id);
        }
    };
} // namespace nitro::rhi
