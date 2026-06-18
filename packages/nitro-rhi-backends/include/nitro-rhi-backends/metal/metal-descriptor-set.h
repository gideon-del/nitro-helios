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
        void writeTexture(RHITexture *texture, uint32_t binding, ImageLayout imageLayout) override;
        void commit() override;

        static constexpr uint32_t c_TEXTURES_PER_SET = 16;
        static constexpr uint32_t c_BUFFER_PER_SET = 16;

        static uint32_t s_getMetalBufferBinding(
            uint32_t set,
            uint32_t binding)
        {
            return set * MetalDescriptorSet::c_BUFFER_PER_SET + binding;
        }

        static uint32_t s_getMetalTextureBinding(
            uint32_t set,
            uint32_t binding)
        {
            return set * MetalDescriptorSet::c_TEXTURES_PER_SET + binding;
        }
        std::unordered_map<MetalTexture *, uint32_t> textureBindings;
        std::unordered_map<MetalBuffer *, uint32_t> bufferBindings;
        MetalDescriptorLayout *descriptorLayout;

    private:
        MetalDevice *m_device;
    };
} // namespace nitro::rhi::metal
