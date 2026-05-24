#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-device.h>

namespace nitro::rhi::metal
{
    MetalBuffer::MetalBuffer(MetalDevice *device, const BufferDesc &desc) : m_device(device), m_size(desc.size)
    {
        if (desc.usage != BufferDesc::Usage::Uniform && desc.initialData != nullptr)
        {
            buffer = m_device->device->newBuffer(
                desc.initialData,
                NS::UInteger(desc.size),
                MTL::ResourceStorageModeShared);
        }
        else
        {
            buffer = m_device->device->newBuffer(
                NS::UInteger(desc.size),
                MTL::ResourceStorageModeShared);
        }
    }

    MetalBuffer::~MetalBuffer()
    {
        if (buffer)
            buffer->release();
    }
    void MetalBuffer::upload(const void *data, size_t size)
    {
        memcpy(buffer->contents(), data, size);
    }

    size_t MetalBuffer::getSize() const
    {
        return m_size;
    }
} // namespace nitro::rhi::metal
