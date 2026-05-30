#include <nitro-renderer/frame-resource.h>

namespace nitro::renderer
{
    FrameResource::FrameResource(rhi::RHIDevice *device) : m_device(device) {}

    FrameResource::~FrameResource()
    {
        }

    void FrameResource::setBuffer(FrameResourceId id, rhi::RHIBuffer *buffer)
    {
        m_buffers[id] = buffer;
    }
    void FrameResource::setTexture(FrameResourceId id, rhi::RHITexture *texture)
    {
        m_textures[id] = texture;
    }
    void FrameResource::setDescriptorSet(FrameResourceId id, rhi::RHIDescriptorSet *descriptorSet)
    {
        m_descriptorSets[id] = descriptorSet;
    }
    rhi::RHIBuffer *FrameResource::getBuffer(FrameResourceId id)
    {
        return m_buffers[id];
    }
    rhi::RHITexture *FrameResource::getTexture(FrameResourceId id)
    {
        return m_textures[id];
    }
    rhi::RHIDescriptorSet *FrameResource::getDescriptorSet(FrameResourceId id)
    {
        return m_descriptorSets[id];
    }
} // namespace nitro::renderer
