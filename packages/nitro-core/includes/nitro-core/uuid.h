#pragma once
#include <random>
#include <cstdint>
#include <optional>

namespace nitro
{
    struct UUID
    {
        uint64_t high = 0;
        uint64_t low = 0;
        UUID(uint64_t high, uint64_t low) : high(high), low(low) {};
        UUID() = default;
        static UUID generate()
        {
            static thread_local std::mt19937_64 rng{std::random_device{}()};
            static thread_local std::uniform_int_distribution<uint64_t> dist;
            UUID u{dist(rng), dist(rng)};
            u.high = (u.high & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;
            u.low = (u.low & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;

            return u;
        };

        std::string toString() const
        {
            char buf[37];
            std::snprintf(buf, sizeof(buf),
                          "%08x-%04x-%04x-%04x-%012llx",
                          uint32_t(high >> 32), uint32_t((high >> 16) & 0xFFFF), uint32_t(high & 0xFFFF),
                          uint32_t(low >> 48), (unsigned long long)(low & 0xFFFFFFFFFFFFull));
            return buf;
        }

        static std::optional<UUID> parse(std::string_view s)
        {
            if (s.size() != 36)
                return std::nullopt;
            std::string hex;
            for (char c : s)
                if (c != '-')
                    hex += c;
            if (hex.size() != 32)
                return std::nullopt;
            return std::make_optional(UUID{std::stoull(hex.substr(0, 16), nullptr, 16),
                                           std::stoull(hex.substr(16, 16), nullptr, 16)});
        }

        bool operator==(const UUID &o) const { return high == o.high && low == o.low; }
    };

} // namespace nitro

namespace std
{

    template <>
    struct hash<nitro::UUID>
    {
        size_t operator()(const nitro::UUID &u) const
        {
            return std::hash<uint64_t>{}(u.high) ^ (std::hash<uint64_t>{}(u.low) << 1);
        }
    };

} // namespace std