#pragma once
#include "handles.h"
#include "nitro-rhi/rhi.h"
#include "per-frame.h"
#include "entity-store.h"
#include "nitro-core/types/pool.h"
namespace nitro::renderer
{
    struct PointLightDesc
    {
        glm::vec4 position{5.0f, 5.0f, 5.0f, 1.0f};
        glm::vec4 color{1.0f, 0.0f, 1.0f, 1.0f};
        float radius = 200.0f;
        float intensity = 1.0f;
        float pad[2];
    };
    struct LightManagerFrameResource
    {
        rhi::RHIBuffer *lightBuffer = nullptr;
        size_t capacity = 1024;
    };
    class LightManager
    {
    public:
        LightManager(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<EntityStore> entityStore);
        ~LightManager();

        void flush();
        PointLightHandle addPointLight(float radius, float intensity);
        PointLight *getPointLight(const PointLightHandle &handle);
        void markPointLightDirty(const PointLightHandle &handle);
        void reclaimSlot(const PointLightHandle &handle);
        void deactivateSlot(const PointLightHandle &handle);
        bool reactivateSlot(const PointLightHandle &handle, PointLight light);
        void clear();

        rhi::RHIBuffer *getPointLightBuffer() { return m_frameResources.current(m_device->getCurrentFrameIndex()).lightBuffer; }
        uint32_t pointLightCount() const { return m_pool.size(); };

    private:
        void markLightBufferDirty();
        void buildPointLightFrameBuffer(uint32_t frameIdx);
        void updatePointLightBuffer(uint32_t frameIdx, const PointLight &pointLight, const PointLightHandle &handle);

        PointLightDesc buildPointLightDesc(const PointLight &light);

    private:
        PerFrame<LightManagerFrameResource> m_frameResources;
        ResourcePool<PointLight> m_pool;
        std::vector<PointLightHandle> m_dirtyPointLights;
        uint8_t m_dirtyPointLightBuffer = 0;
        std::shared_ptr<EntityStore> m_entityStore;
        std::shared_ptr<rhi::RHIDevice> m_device;
    };
} // namespace nitro::renderer
