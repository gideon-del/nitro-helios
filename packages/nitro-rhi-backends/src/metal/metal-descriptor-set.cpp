#include <nitro-rhi-backends/metal/metal-descriptor-set.h>
#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>

namespace nitro::rhi::metal
{
    MetalDescriptorSet::MetalDescriptorSet(MetalDevice *device, MetalDescriptorLayout *descriptorLayout) : m_device(device), descriptorLayout(descriptorLayout) {}

    MetalDescriptorSet::~MetalDescriptorSet() {}

    void MetalDescriptorSet::writeBuffer(RHIBuffer *buffer, uint32_t binding)
    {

        MetalBuffer *mtBuffer = reinterpret_cast<MetalBuffer *>(buffer);

        m_tempDescriptorSet.bufferBindings[mtBuffer] = binding;
    }
    void MetalDescriptorSet::writeTexture(RHITexture *texture, uint32_t binding, ImageLayout imageLayout)
    {

        MetalTexture *mtTexture = reinterpret_cast<MetalTexture *>(texture);

        m_tempDescriptorSet.textureBindings[mtTexture] = binding;
    }

    void MetalDescriptorSet::commit()
    {
        textureBindings.clear();
        bufferBindings.clear();

        textureBindings = m_tempDescriptorSet.textureBindings;
        bufferBindings = m_tempDescriptorSet.bufferBindings;

        m_tempDescriptorSet.textureBindings.clear();
        m_tempDescriptorSet.bufferBindings.clear();
    };

} // namespace nitro::rhi::metal
