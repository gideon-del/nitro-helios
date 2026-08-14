#pragma once
#include "mesh-manager.h"
#include "material-manager.h"
#include <vector>
#include <nitro-rhi/rhi.h>

namespace nitro::renderer
{
    struct Scene
    {

        Scene(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<MeshManager> meshManager, std::shared_ptr<MaterialManager> materialManager)
            : m_device(std::move(device)), meshManager(std::move(meshManager)), materialManager(std::move(materialManager)) {}

        ~Scene()
        {
            if (m_sceneInstanceIdBuffer)
                m_device->destroyBuffer(m_sceneInstanceIdBuffer);
        }

        void draw(rhi::RHICommandBuffer *cmd, rhi::RHIBuffer *drawCommandsBuffer, rhi::RHIBuffer *drawCountBuffer)
        {

            uint32_t drawCount = static_cast<uint32_t>(instanceIds.size());
            cmd->bindVertexBuffer(meshManager->getVertexMegaBuffer());
            cmd->bindIndexBuffer(meshManager->getIndexMegaBuffer());

            auto mapped = drawCountBuffer->map();

            memcpy(&drawCount, mapped, sizeof(uint32_t));
            drawCountBuffer->unmap();

            drawCount = std::min(drawCount, static_cast<uint32_t>(instanceIds.size()));

            std::cout << "Draw Count " << drawCount << std::endl;
            cmd->drawIndexedIndirect(
                drawCommandsBuffer,
                0,
                drawCount,
                sizeof(rhi::DrawIndexedIndirectArgs));

            // cmd->drawIndexedIndirectCount(
            //     drawCommandsBuffer,
            //     0,
            //     drawCountBuffer,
            //     0,
            //     meshManager->instanceCount(),
            //     sizeof(rhi::DrawIndexedIndirectArgs));
        };
        void buildSceneInstanceId()
        {
            if (instanceIds.empty())
                return;

            rhi::BufferDesc desc;
            desc.initialData = instanceIds.data();
            desc.size = sizeof(uint32_t) * instanceIds.size();
            desc.storage = rhi::BufferDesc::StorageMode::GPU;
            desc.usage = rhi::BufferDesc::Usage::Storage;

            if (m_sceneInstanceIdBuffer)
                m_device->destroyBuffer(m_sceneInstanceIdBuffer);
            m_sceneInstanceIdBuffer = m_device->createBuffer(desc);
        }
        rhi::RHIBuffer *sceneInstanceIdBuffer() { return m_sceneInstanceIdBuffer; }

        void loadGltfScene(std::string filePath, std::shared_ptr<rhi::RHIDevice> device);

        std::shared_ptr<MeshManager> meshManager;
        std::shared_ptr<MaterialManager> materialManager;
        std::vector<uint32_t> instanceIds;
        static constexpr uint32_t s_MAX_DRAW_COMMANDS = 100000;

    private:
        rhi::RHIBuffer *m_sceneInstanceIdBuffer = nullptr;
        std::shared_ptr<rhi::RHIDevice> m_device;
    };
} // namespace nitro::renderer
