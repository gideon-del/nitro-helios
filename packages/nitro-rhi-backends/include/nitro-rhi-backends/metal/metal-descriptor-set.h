#pragma once
#include <unordered_map>
#include <nitro-rhi/rhi-descriptor-set.h>

namespace nitro::rhi::metal
{

    class MetalDevice;
    class MetalBuffer;
    class MetalTexture;
    class MetalDescriptorLayout;
    class MetalDescriptorSet : public RHIDescriptorSet
    {
    public:
        MetalDescriptorSet(MetalDevice *device, MetalDescriptorLayout *descriptorLayout);
        ~MetalDescriptorSet() override;

        void writeBuffer(RHIBuffer *buffer, uint32_t binding) override;
        void writeTexture(RHITexture *texture, uint32_t binding) override;
        void commit() override;

        std::unordered_map<MetalTexture *, uint32_t> textureBindings;
        std::unordered_map<MetalBuffer *, uint32_t> bufferBindings;
        MetalDescriptorLayout *descriptorLayout;

    private:
        MetalDevice *m_device;
    };
} // namespace nitro::rhi::metal
