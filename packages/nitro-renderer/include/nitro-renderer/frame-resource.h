#pragma once
#include <unordered_map>
#include <nitro-rhi/rhi-buffer.h>
#include <nitro-rhi/rhi-descriptor-set.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-texture.h>

namespace nitro::renderer
{
    enum class FrameResourceId
    {
        GlobalUniformBuffer,
        LightUniformBuffer,
        DirectionalLightUniformBuffer,
        FrameDataUniformBuffer,
        CameraViewUniformBuffer,
        MainDescriptorSet,
        ShadowDescriptorSet,
        ShadowTexture,
        TextureDescriptor,
    };

    class FrameResource
    {

    public:
        FrameResource(rhi::RHIDevice *device);
        ~FrameResource();

        void setBuffer(FrameResourceId id, rhi::RHIBuffer *buffer);
        void setTexture(FrameResourceId id, rhi::RHITexture *texture);
        void setDescriptorSet(FrameResourceId id, rhi::RHIDescriptorSet *descriptorSet);

        rhi::RHIBuffer *getBuffer(FrameResourceId id);
        rhi::RHITexture *getTexture(FrameResourceId id);
        rhi::RHIDescriptorSet *getDescriptorSet(FrameResourceId id);

    private:
        std::unordered_map<FrameResourceId, rhi::RHIBuffer *> m_buffers;
        std::unordered_map<FrameResourceId, rhi::RHITexture *> m_textures;
        std::unordered_map<FrameResourceId, rhi::RHIDescriptorSet *> m_descriptorSets;
        rhi::RHIDevice *m_device;
    };
}