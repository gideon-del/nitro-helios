#pragma once
#include <type_traits>
#include <deque>

namespace nitro::rhi
{
    template <typename T>
    class DeferredDeletionQueue
    {

        static_assert(std::is_nothrow_move_constructible_v<T>, "items are moved in and out");
        struct PendingDeletionPoint
        {
            T item;
            uint64_t retireAt;
        };

    public:
        void push(T &&item, uint64_t retireAt)
        {
            m_deletionQueue.push_back({.item = std::move(item),
                                       .retireAt = retireAt});
        }

        void collect(uint64_t completed)
        {
            while (!m_deletionQueue.empty() && m_deletionQueue.front().retireAt <= completed)
                m_deletionQueue.pop_front();
        }

        void collectAll()
        {
            collect(UINT64_MAX);
        }

    private:
        std::deque<PendingDeletionPoint> m_deletionQueue;
    };
} // namespace nitro::rhi
