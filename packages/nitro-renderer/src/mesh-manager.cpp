#include "nitro-renderer/mesh-manager.h"
#include "meshoptimizer.h"

namespace nitro::renderer
{
    MeshLOD generateMeshLOD(geometry::Mesh &mesh, float ratio)
    {

        MeshLOD lod;

        size_t targetIndexCount =
            static_cast<size_t>(
                mesh.indices.size() * ratio);

        targetIndexCount =
            (targetIndexCount / 3) * 3;

        lod.indices.resize(mesh.indices.size());

        size_t indexCount = meshopt_simplify(
            lod.indices.data(),
            mesh.indices.data(),
            mesh.indices.size(),
            reinterpret_cast<const float *>(
                mesh.vertices.data()),
            mesh.vertices.size(),
            sizeof(geometry::Vertex),

            targetIndexCount,
            0.01f,
            0,
            nullptr);

        lod.indices.resize(indexCount);

        return lod;
    };

    MeshManager::MeshManager(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<assets::AssetManager> assetManager) : m_device(device),
                                                                                                                           m_assetManager(std::move(assetManager))
    {

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [](uint32_t frameIdx)
            {
                return MeshManagerResource{};
            });
    };
    MeshManager::~MeshManager()
    {

        for (auto &resource : m_resources)
        {
            if (resource.instanceBuffer)
            {
                m_device->destroyBuffer(resource.instanceBuffer);
            }
            if (resource.descriptorBuffer)
            {
                m_device->destroyBuffer(resource.descriptorBuffer);
            }
            if (resource.vertexBuffer)
            {
                m_device->destroyBuffer(resource.vertexBuffer);
            }
            if (resource.indexBuffer)
            {
                m_device->destroyBuffer(resource.indexBuffer);
            }
        }
    }

    GPUMeshHandle MeshManager::addMesh(geometry::Mesh mesh)
    {
        HandleValueType id = static_cast<HandleValueType>(m_gpuMeshes.size());
        auto handle = m_assetManager->registerMesh(std::make_unique<Mesh>(mesh));
        return addMeshFromAsset(handle);
    }

    GPUMeshHandle MeshManager::addMeshFromAsset(MeshAssetHandle assetHandle)
    {
        if (auto it = m_assetHandleToGPUHandle.find(assetHandle); it != m_assetHandleToGPUHandle.end())
        {
            return it->second;
        }
        auto mesh = m_assetManager->getAsset(assetHandle);
        if (!mesh)
            return {};

        MeshGpuInfo info;
        info.meshHandle = assetHandle;

        constexpr float LOD_RATIOS[MESH_LOD_COUT] =
            {
                1.0f,
                0.5f,
                0.25f,
                0.125f};

        constexpr float LOD_SCREEN_THRESHOLDS[MESH_LOD_COUT] = {
            1.0f,
            0.15f,
            0.07f,
            0.03f};
        MeshLOD lod0;
        lod0.indices = mesh->indices;
        lod0.screenThreshold = LOD_SCREEN_THRESHOLDS[0];

        info.meshLod[0] = std::move(lod0);

        for (int i = 1; i < MESH_LOD_COUT; i++)
        {
            info.meshLod[i] = generateMeshLOD(*mesh, LOD_RATIOS[i]);

            info.meshLod[i].screenThreshold = LOD_SCREEN_THRESHOLDS[i];
        };

        m_gpuMeshes.push_back(info);
        markMeshDescriptorDirty();
        GPUMeshHandle handle{static_cast<HandleValueType>(m_gpuMeshes.size() - 1)};
        m_assetHandleToGPUHandle[assetHandle] = handle;
        return handle;
    }

    MeshInstanceHandle MeshManager::addMeshInstances(MeshInstance &instance)
    {

        auto gpuMesh = getGPUMesh(instance.mesh);
        assert(gpuMesh != nullptr);
        auto mesh = m_assetManager->getAsset(gpuMesh->meshHandle);
        geometry::MeshTransformation::computeWorldAABB(
            instance.transformation.getTransform().model,
            mesh->aabbMin,
            mesh->aabbMax,
            instance.worldAABBMin,
            instance.worldAABBMax);
        markMeshInstanceBufferDirty();
        return m_instances.emplace(std::move(instance));
    }

    void MeshManager::buildMegaVertexBuffer(uint32_t frameIdx)
    {
        auto &resource = m_resources.current(frameIdx);
        if (m_gpuMeshes.empty())
            return;

        // pass 1: assign offsets, measure total
        uint32_t cursor = 0;
        for (auto &info : m_gpuMeshes)
        {

            const auto *mesh = m_assetManager->getAsset(info.meshHandle);
            if (!mesh)
                continue;
            info.vertexOffset = cursor;
            cursor += static_cast<uint32_t>(mesh->vertices.size());
        }

        std::vector<geometry::Vertex> vertices(cursor);
        for (auto &info : m_gpuMeshes)
        {

            const auto *mesh = m_assetManager->getAsset(info.meshHandle);
            if (!mesh)
                continue;
            memcpy(vertices.data() + info.vertexOffset,
                   mesh->vertices.data(),
                   sizeof(geometry::Vertex) * mesh->vertices.size());
        }

        if (resource.vertexBuffer)
            m_device->destroyBuffer(resource.vertexBuffer);
        rhi::BufferDesc desc;
        desc.initialData = vertices.data();
        desc.size = sizeof(geometry::Vertex) * cursor;
        desc.storage = rhi::BufferDesc::StorageMode::GPU;
        desc.usage = rhi::BufferDesc::Usage::Vertex;
        resource.vertexBuffer = m_device->createBuffer(desc);
    }

    void MeshManager::buildMegaIndexBuffer(uint32_t frameIdx)
    {
        auto &resource = m_resources.current(frameIdx);
        if (m_gpuMeshes.size() == 0)
            return;
        if (resource.indexBuffer)
            m_device->destroyBuffer(resource.indexBuffer);

        uint32_t cursor = 0;

        for (auto &info : m_gpuMeshes)
        {
            auto mesh = m_assetManager->getAsset(info.meshHandle);

            for (int i = 0; i < MESH_LOD_COUT; i++)
            {
                info.meshLod[i].indexOffset = cursor;
                cursor += static_cast<uint32_t>(info.meshLod[i].indices.size());
            }
        }
        std::vector<uint32_t> indices(cursor);
        for (auto &info : m_gpuMeshes)
        {
            for (int i = 0; i < MESH_LOD_COUT; i++)
            {
                memcpy(indices.data() + info.meshLod[i].indexOffset, info.meshLod[i].indices.data(), sizeof(uint32_t) * info.meshLod[i].indices.size());
            }
        }

        rhi::BufferDesc indexDesc;
        indexDesc.initialData = indices.data();
        indexDesc.size = sizeof(uint32_t) * cursor;
        indexDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        indexDesc.usage = rhi::BufferDesc::Usage::Index;

        resource.indexBuffer = m_device->createBuffer(indexDesc);
    }

    void MeshManager::buildMegaBuffers()
    {

        for (uint32_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++)
        {
            rebuildFrameInstanceBuffer(i);
            buildMegaVertexBuffer(i);
            buildMegaIndexBuffer(i);
            buildMeshDescriptorBuffer(i);
        }

        m_dirtyDescriptorBufferMask = 0;
        m_dirtyInstanceBufferMask = 0;
    }

    MeshGpuInfo *MeshManager::getGPUMesh(const GPUMeshHandle &handle)
    {
        if (!handle.isValid() || handle.index >= m_gpuMeshes.size())
        {
            return nullptr;
        }

        return &m_gpuMeshes[handle.index];
    }

    MeshInstance *MeshManager::getMeshInstance(const MeshInstanceHandle handle)
    {

        return m_instances.get(handle);
    }

    void MeshManager::markMeshInstanceAsDirty(const MeshInstanceHandle &handle)
    {
        auto instance = getMeshInstance(handle);
        if (!instance)
            return;

        if (instance->dirtyMask == 0)
            m_dirtyInstances.push_back(handle);
        instance->dirtyMask = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
    }

    void MeshManager::flusDirtyMeshInstances()
    {
        auto frameIdx = m_device->getCurrentFrameIndex();
        const uint8_t bit = 1 << frameIdx;

        if (m_dirtyDescriptorBufferMask & bit)
        {
            buildMegaIndexBuffer(frameIdx);
            buildMegaVertexBuffer(frameIdx);
            buildMeshDescriptorBuffer(frameIdx);
            m_dirtyDescriptorBufferMask &= ~bit;
        }
        if (m_dirtyInstanceBufferMask & bit)
        {
            rebuildFrameInstanceBuffer(frameIdx);
            m_dirtyInstanceBufferMask &= ~bit;
        }
        uint write = 0;
        for (auto &h : m_dirtyInstances)
        {
            auto instance = getMeshInstance(h);

            if (!instance)
                continue;

            if (instance->dirtyMask & bit)
            {

                instance->dirtyMask &= ~bit;
                updateMeshInstanceBuffer(*instance, h.index, frameIdx);
            }

            if (instance->dirtyMask != 0)
            {
                m_dirtyInstances[write++] = h;
            }
        }

        m_dirtyInstances.resize(write);
    }

    MeshInstanceDesc MeshManager::createInstanceDesc(MeshInstance &instance)
    {
        MeshInstanceDesc desc;

        desc.meshId = instance.mesh.index;

        desc.materialId = instance.material.isValid() ? instance.material.index : INVALID_MATERIAL_INDEX;
        auto pc = instance.transformation.getTransform();
        desc.modelTransform = pc.model;
        desc.normalTransform = pc.normalMatrix;

        return desc;
    }

    void MeshManager::updateMeshInstanceBuffer(MeshInstance &instance, uint32_t id, uint32_t frameIdx)
    {

        MeshInstanceDesc desc = createInstanceDesc(instance);

        auto &resource = m_resources.current(frameIdx);

        resource.instanceBuffer->upload(
            &desc,
            sizeof(MeshInstanceDesc),
            static_cast<size_t>(id) * sizeof(MeshInstanceDesc));
    }

    void MeshManager::rebuildFrameInstanceBuffer(uint32_t frameIdx)
    {
        auto &resource = m_resources.current(frameIdx);
        const size_t needed = m_instances.capacity();

        if (!resource.instanceBuffer || needed > resource.instanceCapacity)
        {
            m_device->waitIdle();
            if (resource.instanceBuffer)
                m_device->destroyBuffer(resource.instanceBuffer);

            if (needed > resource.instanceCapacity)
                resource.instanceCapacity = std::max(needed * 2, size_t(1024));

            rhi::BufferDesc desc;
            desc.size = sizeof(MeshInstanceDesc) * resource.instanceCapacity;
            desc.storage = rhi::BufferDesc::StorageMode::Dynamic;
            desc.usage = rhi::BufferDesc::Usage::Storage;
            resource.instanceBuffer = m_device->createBuffer(desc);
        }

        std::vector<MeshInstanceDesc> descs(resource.instanceCapacity);
        m_instances.forEach([&](MeshInstanceHandle h, MeshInstance &inst)
                            { descs[h.index] = createInstanceDesc(inst); });

        resource.instanceBuffer->upload(descs.data(),
                                        sizeof(MeshInstanceDesc) * resource.instanceCapacity, 0);
    };
    void MeshManager::buildMeshDescriptorBuffer(uint32_t frameIdx)
    {
        auto &resource = m_resources.current(frameIdx);
        const size_t needed = m_gpuMeshes.size();

        if (!resource.descriptorBuffer || needed > resource.meshCapacity)
        {
            m_device->waitIdle();
            if (resource.descriptorBuffer)
                m_device->destroyBuffer(resource.descriptorBuffer);

            if (needed > resource.meshCapacity)
                resource.meshCapacity = std::max(needed * 2, size_t(1024));

            rhi::BufferDesc desc;
            desc.size = sizeof(MeshDescriptor) * resource.meshCapacity;
            desc.storage = rhi::BufferDesc::StorageMode::Dynamic;
            desc.usage = rhi::BufferDesc::Usage::Storage;
            resource.descriptorBuffer = m_device->createBuffer(desc);
        }

        std::vector<MeshDescriptor> descriptors;
        descriptors.reserve(resource.meshCapacity);
        for (auto &meshInfo : m_gpuMeshes)
        {
            auto mesh = m_assetManager->getAsset(meshInfo.meshHandle);
            assert(mesh != nullptr);
            MeshDescriptor descriptor{};
            descriptor.indexOffset = meshInfo.meshLod[MESH_LOD_COUT - 1].indexOffset;
            // descriptor.indexCount = static_cast<uint32_t>(meshInfo.mesh.indices.size());
            descriptor.vertexOffset = meshInfo.vertexOffset;
            descriptor.aabbMax = mesh->aabbMax;
            descriptor.aabbMin = mesh->aabbMin;
            descriptor.boundingSphereRadius = mesh->boundingSphereRadius;

            for (int i = 0; i < MESH_LOD_COUT; i++)
            {
                MeshLODDescriptor lodDescriptor{};
                lodDescriptor.indexCount = static_cast<uint32_t>(meshInfo.meshLod[i].indices.size());
                lodDescriptor.indexOffset = static_cast<uint32_t>(meshInfo.meshLod[i].indexOffset);
                lodDescriptor.screenThreshold = meshInfo.meshLod[i].screenThreshold;

                descriptor.lod[i] = std::move(lodDescriptor);
            }

            descriptors.push_back(std::move(descriptor));
        }

        resource.descriptorBuffer->upload(descriptors.data(),
                                          sizeof(MeshDescriptor) * resource.meshCapacity, 0);
    };

    void MeshManager::reclaimInstance(const MeshInstanceHandle &handle)
    {
        m_instances.reclaim(handle);
    };
    void MeshManager::deactivateMeshInstance(const MeshInstanceHandle &handle)
    {
        m_instances.deactivate(handle);
        m_dirtyInstanceBufferMask = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
    };
    bool MeshManager::reactivateMeshInstance(MeshInstanceHandle &handle, MeshInstance &instance)
    {
        MeshInstance copied = instance;
        bool ok = m_instances.reactivate(handle, std::move(copied));
        if (ok)
            m_dirtyInstanceBufferMask = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
        return ok;
    }

    void MeshManager::clear()
    {
        m_gpuMeshes.clear();
        m_assetHandleToGPUHandle.clear();
        m_instances.clear();
        m_dirtyInstances.clear();
    };

} // namespace nitro::renderer
