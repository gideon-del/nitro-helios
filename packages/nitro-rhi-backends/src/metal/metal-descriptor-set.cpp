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

        bufferBindings[mtBuffer] = binding;
    }
    void MetalDescriptorSet::writeTexture(RHITexture *texture, uint32_t binding)
    {

        MetalTexture *mtTexture = reinterpret_cast<MetalTexture *>(texture);

        textureBindings[mtTexture] = binding;
    }

    void MetalDescriptorSet::commit() {};

} // namespace nitro::rhi::metal
