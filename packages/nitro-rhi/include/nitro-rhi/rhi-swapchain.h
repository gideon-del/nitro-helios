#pragma once
#include "rhi-texture.h"
#include <cstdint>
#include <vector>
#include <functional>
namespace nitro::rhi
{

    typedef std::function<void(uint32_t, uint32_t)> ResizeCallback;

    struct RHIViewScale
    {
        float x = 1.0f;
        float y = 1.0f;
    };
    class RHISwapchain
    {
    public:
        virtual ~RHISwapchain() = default;
        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual uint32_t getWidth() = 0;
        virtual uint32_t getHeight() = 0;
        virtual RHITexture *getCurrentBackbuffer() = 0;
        virtual RHIViewScale getViewScale() = 0;
        void addResizeCallback(ResizeCallback callback)
        {
            m_resizeCallbacks.push_back(callback);
        };

        void notifyResizeCallbacks(uint32_t width, uint32_t height)
        {

            for (auto &cb : m_resizeCallbacks)
            {
                cb(width, height);
            }
        }

    private:
        std::vector<ResizeCallback> m_resizeCallbacks;
    };
}