#pragma once
#include "handle.h"
#include <cstddef>
#include <vector>
#include <new>
#include <utility>

namespace nitro
{
    template <typename T>
    class ResourcePool
    {
        using PoolHandle = Handle<T>;

        struct Slot
        {
            alignas(T) std::byte storage[sizeof(T)];
            uint32_t generation = 0;
            bool alive = false;
        };

        std::vector<Slot> m_slots;
        std::vector<uint32_t> m_free;

    public:
        ResourcePool() = default;
        ResourcePool(const ResourcePool &) = delete;
        ResourcePool &operator=(const ResourcePool &) = delete;
        ResourcePool(ResourcePool &&) = default;
        ResourcePool &operator=(ResourcePool &&) = default;
        ~ResourcePool() { clear(); }

        template <class... Args>
        PoolHandle emplace(Args &&...args)
        {

            if (!m_free.empty())
            {
                uint32_t idx = m_free.back();
                m_free.pop_back();
                new (&m_slots[idx].storage) T{std::forward<Args>(args)...};
                m_slots[idx].alive = true;
                return {idx, m_slots[idx].generation};
            }

            m_slots.emplace_back();
            uint32_t idx = uint32_t(m_slots.size() - 1);
            new (&m_slots[idx].storage) T{std::forward<Args>(args)...};
            m_slots[idx].alive = true;
            return {idx, m_slots[idx].generation};
        };

        void free(PoolHandle h)
        {

            if (!isAlive(h))
                return;

            get(h)->~T();

            m_slots[h.id].alive = false;
            ++m_slots[h.id].generation;
            m_free.push_back(h.id);
        };
        void deactivate(PoolHandle h)
        {

            if (!isAlive(h))
                return;

            get(h)->~T();

            m_slots[h.id].alive = false;
        };
        template <class... Args>
        bool reactivate(PoolHandle h, Args &&...args)
        {
            if (!h.isValid() || h.id >= m_slots.size())
                return false;
            Slot &s = m_slots[h.id];
            if (s.alive || h.generation != s.generation)
                return false;
            new (&s.storage) T(std::forward<Args>(args)...);
            s.alive = true;
            return true;
        }

        void reclaim(PoolHandle h)
        {
            if (!h.isValid() || h.id >= m_slots.size())
                return;
            Slot &s = m_slots[h.id];
            if (s.alive || h.generation != s.generation)
                return;
            ++s.generation;
            m_free.push_back(h.id);
        }

        template <class Fn>
        void forEach(Fn &&callback) const
        {
            for (uint32_t i = 0; i < m_slots.size(); i++)
            {
                auto &slot = m_slots[i];
                if (!slot.alive)
                {
                    continue;
                }

                PoolHandle h{i, slot.generation};
                auto value = get(h);
                callback(h, *value);
            }
        }
        template <class Fn>
        void forEach(Fn &&callback)
        {
            for (uint32_t i = 0; i < m_slots.size(); i++)
            {
                auto &slot = m_slots[i];
                if (!slot.alive)
                {
                    continue;
                }

                PoolHandle h{i, slot.generation};
                auto value = get(h);
                callback(h, *value);
            }
        }

        T *get(PoolHandle h)
        {
            if (!isAlive(h))
                return nullptr;
            return std::launder(reinterpret_cast<T *>(&m_slots[h.id].storage));
        }
        const T *get(PoolHandle h) const
        {
            if (!isAlive(h))
                return nullptr;
            return std::launder(reinterpret_cast<T *>(&m_slots[h.id].storage));
        }
        size_t capacity() const { return m_slots.size(); }
        size_t size() const { return m_slots.size() - m_free.size(); }

    private:
        bool isAlive(PoolHandle h) const
        {
            if (!h.isValid() || h.id >= m_slots.size())
                return false;

            const Slot &slot = m_slots[h.id];

            return slot.alive && h.generation == slot.generation;
        }
        void clear()
        {
            for (auto &s : m_slots)
                if (s.alive)
                {
                    std::launder(reinterpret_cast<T *>(&s.storage))->~T();
                    s.alive = false;
                }
            m_slots.clear();
            m_free.clear();
        }
    };
} // namespace nitro
