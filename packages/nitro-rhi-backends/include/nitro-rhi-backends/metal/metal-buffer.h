#pragma once
#include <nitro-rhi/rhi-buffer.h>
#include <SingleHeader/MetalCpp.h>

namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalBuffer : public RHIBuffer
    {
    public:
        MetalBuffer(MetalDevice *device, const BufferDesc &desc);
        ~MetalBuffer() override;
        void upload(const void *data, size_t size) override;
        size_t getSize() const override;

        MTL::Buffer *buffer;

    private:
        MetalDevice *m_device;
        size_t m_size;
    };
} // namespace nitro::rhi::metal
