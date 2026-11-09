#pragma once
#include <vector>
namespace nitro::frame
{
    template <typename T>
    class MultiBuffer
    {
        std::vector<T> m_slots;
        size_t m_count = 0;

    public:
        template <class F>
        void create(size_t count, F &&fn)
        {

            m_count = count;
            m_slots.clear();
            for (int i = 0; i < count; i++)
            {
                m_slots.push_back(std::move(fn(i)));
            }
        };

        void clear()
        {
            m_slots.clear();
            m_count = 0;
        }

        T *get(uint32_t frameIdx = 0)
        {
            if (m_slots.empty() || m_count == 0)
                return nullptr;

            auto idx = frameIdx % m_count;
            return &m_slots[idx];
        }

        template <class F>
        void forEach(F &&fn)
        {
            for (auto &slot : m_slots)
            {
                fn(slot);
            }
        }

        bool empty() const
        {
            return m_slots.empty();
        }

        ~MultiBuffer()
        {
            m_slots.clear();
        }
    };
} // namespace nitro::frame
