#pragma once
#include <vector>
#include <cstdint>
namespace nitro::renderer
{
    template <typename T>
    class PerFrame
    {
    public:
        template <typename Factory>
        void create(uint32_t frames, Factory factory)
        {
            for (uint32_t i = 0; i < frames; i++)
            {
                m_resources.push_back(factory(i));
            }
        };

        T &current(uint32_t frame) { return m_resources[frame]; }
        auto begin() { return m_resources.begin(); }
        auto end() { return m_resources.end(); }

        auto begin() const { return m_resources.begin(); }
        auto end() const { return m_resources.end(); }

    private:
        std::vector<T> m_resources;
    };

    static constexpr uint32_t g_MAX_FRAMES_IN_FLIGHT = 2;
} // namespace nitro::renderer
