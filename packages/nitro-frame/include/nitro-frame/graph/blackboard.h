#pragma once
#include <any>
#include <unordered_map>
#include <atomic>
#include <cstdint>
#include <type_traits>
#include <cassert>
#include <utility>

namespace nitro::frame
{

    using TypeId = uint32_t;

    inline TypeId nextTypeId()
    {
        static std::atomic<TypeId> counter{0};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

    template <typename T>
    TypeId typeIdImpl()
    {
        static const TypeId id = nextTypeId();
        return id;
    }
    template <typename T>
    TypeId typeId()
    {
        return typeIdImpl<std::remove_cvref_t<T>>();
    }
    class Blackboard
    {

        std::unordered_map<TypeId, std::any> m_entries;

    public:
        template <typename T, typename... Args>
        std::remove_cvref_t<T> &add(Args &&...args)
        {

            using U = std::remove_cvref_t<T>;
            static_assert(std::is_copy_constructible_v<U>, "std::any requires copyable types");

            auto [it, inserted] = m_entries.try_emplace(
                typeId<U>(), std::in_place_type<U>, std::forward<Args>(args)...);
            assert(inserted && "This type has already been added");

            return std::any_cast<U &>(it->second);
        }
        template <typename T>
        std::remove_cvref_t<T> &get()
        {
            using U = std::remove_cvref_t<T>;
            auto it = m_entries.find(typeId<U>());

            assert(it != m_entries.end() && "Type not found in entries");

            return std::any_cast<U &>(it->second);
        }
        template <typename T>
        const std::remove_cvref_t<T> &get() const
        {
            using U = std::remove_cvref_t<T>;
            auto it = m_entries.find(typeId<U>());

            assert(it != m_entries.end() && "Type not found in entries");

            return std::any_cast<const U &>(it->second);
        }
        template <typename T>
        std::remove_cvref_t<T> *tryGet()
        {
            using U = std::remove_cvref_t<T>;
            auto it = m_entries.find(typeId<U>());
            if (it == m_entries.end())
                return nullptr;

            return std::any_cast<U>(&it->second);
        }
        template <typename T>
        const std::remove_cvref_t<T> *tryGet() const
        {
            using U = std::remove_cvref_t<T>;
            auto it = m_entries.find(typeId<U>());
            if (it == m_entries.end())
                return nullptr;

            return std::any_cast<U>(&it->second);
        }

        void clear()
        {
            m_entries.clear();
        }
    };
} // namespace nitro::frame
