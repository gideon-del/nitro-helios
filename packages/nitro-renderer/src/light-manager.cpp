#include "nitro-renderer/light-manager.h"

namespace nitro::renderer
{
    LightManager::LightManager(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<EntityStore> entityStore)
        : m_device(std::move(device)),
          m_entityStore(std::move(entityStore))
    {
        m_frameResources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frameIdx)
            {
                LightManagerFrameResource resource;

                rhi::BufferDesc desc;
                desc.size = resource.capacity * sizeof(PointLightDesc);
                desc.storage = rhi::BufferDesc::StorageMode::Dynamic;
                desc.usage = rhi::BufferDesc::Usage::Storage;

                resource.lightBuffer = m_device->createBuffer(desc);

                return resource;
            });
    }

    LightManager::~LightManager()
    {
        m_pool.clear();
        for (auto &resource : m_frameResources)
        {
            if (resource.lightBuffer)
                m_device->destroyBuffer(resource.lightBuffer);
        }
    }

    PointLightHandle LightManager::addPointLight(float radius, float intensity)
    {
        PointLight light;
        light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        light.intensity = intensity;
        light.radius = radius;

        auto handle = m_pool.emplace(std::move(light));

        markLightBufferDirty();
        return handle;
    };

    void LightManager::markLightBufferDirty()
    {
        m_dirtyPointLightBuffer = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
    }
    PointLight *LightManager::getPointLight(const PointLightHandle &handle)
    {
        return m_pool.get(handle);
    }

    void LightManager::markPointLightDirty(const PointLightHandle &handle)
    {
        // auto pointLight = getPointLight(handle);
        // if (!pointLight)
        //     return;

        // if (pointLight->dirtyMask == 0)
        //     m_dirtyPointLights.push_back(handle);
        // pointLight->dirtyMask = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;

        markLightBufferDirty();
    }

    void LightManager::reclaimSlot(const PointLightHandle &handle)
    {
        m_pool.reclaim(handle);
    }

    void LightManager::deactivateSlot(const PointLightHandle &handle)
    {
        m_pool.deactivate(handle);
        markLightBufferDirty();
    }

    bool LightManager::reactivateSlot(const PointLightHandle &handle, PointLight light)
    {
        auto ok = m_pool.reactivate(handle, std::move(light));

        if (ok)
            markLightBufferDirty();

        return ok;
    }
    void LightManager::buildPointLightFrameBuffer(uint32_t frameIdx)
    {

        size_t neededSize = m_pool.size();
        auto &resource = m_frameResources.current(frameIdx);

        if (neededSize > resource.capacity)
        {
            resource.capacity = std::max(neededSize * 2, size_t(1024));

            rhi::BufferDesc desc;
            desc.size = resource.capacity * sizeof(PointLightDesc);
            desc.storage = rhi::BufferDesc::StorageMode::Dynamic;
            desc.usage = rhi::BufferDesc::Usage::Storage;

            resource.lightBuffer = m_device->createBuffer(desc);
        };

        std::vector<PointLightDesc> lightDescriptors;

        m_pool.forEach(
            [&](const PointLightHandle &handle, const PointLight &light)
            {
                PointLightDesc desc = buildPointLightDesc(light);
                lightDescriptors.push_back(std::move(desc));
            });

        resource.lightBuffer->upload(
            lightDescriptors.data(),
            sizeof(PointLightDesc) * lightDescriptors.size());
    };

    PointLightDesc LightManager::buildPointLightDesc(const PointLight &light)
    {
        PointLightDesc desc;

        desc.color = light.color;

        desc.intensity = light.intensity;
        desc.radius = std::max(light.radius, 0.001f);
        auto entity = m_entityStore->get(light.entity);

        if (entity)
        {
            desc.position = glm::vec4(entity->transformation.baseTranslation(), 1.0f);
        }
        return desc;
    }
    void LightManager::updatePointLightBuffer(uint32_t frameIdx, const PointLight &pointLight, const PointLightHandle &handle)
    {
        auto &resource = m_frameResources.current(frameIdx);

        if (!resource.lightBuffer || handle.index >= resource.capacity)
            return;

        auto desc = buildPointLightDesc(pointLight);

        resource.lightBuffer->upload(
            &desc,
            sizeof(PointLightDesc),
            static_cast<size_t>(handle.index) * sizeof(PointLightDesc));
    }

    void LightManager::clear()
    {
        m_pool.clear();
        m_dirtyPointLights.clear();
    };

    void LightManager::flush()
    {
        auto frameIdx = m_device->getCurrentFrameIndex();
        auto bit = 1 << frameIdx;

        if (m_dirtyPointLightBuffer & bit)
        {
            std::cout << "Built Light buffer again " << frameIdx << std::endl;
            buildPointLightFrameBuffer(frameIdx);
            m_dirtyPointLightBuffer &= ~bit;
        }

        uint32_t totalDirty = 0;
        for (auto &handle : m_dirtyPointLights)
        {
            auto pointLight = m_pool.get(handle);

            if (!pointLight)
                continue;

            if (pointLight->dirtyMask & bit)
            {
                updatePointLightBuffer(frameIdx, *pointLight, handle);
                pointLight->dirtyMask &= ~bit;
            }

            if (pointLight->dirtyMask != 0)
            {
                m_dirtyPointLights[totalDirty++] = handle;
            }
        }

        m_dirtyPointLights.resize(totalDirty);
    };

} // namespace nitro::renderer
