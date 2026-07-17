#pragma once
#include <unordered_map>
#include <nitro-rhi/rhi-descriptor-set.h>
#include <Metal/Metal.hpp>

namespace nitro::rhi::metal
{

    class MetalDevice;
    class MetalBuffer;
    class MetalTexture;
    class MetalDescriptorLayout;

    struct TempDescriptorSet
    {
        std::unordered_map<MetalTexture *, uint32_t> textureBindings;
        std::unordered_map<MetalBuffer *, uint32_t> bufferBindings;
        std::unordered_map<MTL::Texture *, uint32_t> storageTextureBindings;
    };
    class MetalDescriptorSet : public RHIDescriptorSet
    {
    public:
        MetalDescriptorSet(MetalDevice *device, MetalDescriptorLayout *descriptorLayout);
        ~MetalDescriptorSet() override;

        void writeBuffer(RHIBuffer *buffer, uint32_t binding) override;
        void writeTexture(const TextureBinding &textureBinding, uint32_t binding, ImageLayout imageLayout) override;
        void writeStorageImage(RHITexture *texture, uint32_t binding, ImageLayout imageLayout, TextureSubresource subresource) override;
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
        std::unordered_map<MTL::Texture *, uint32_t> storageTextureBindings;

        MetalDescriptorLayout *descriptorLayout;

    private:
        MetalDevice *m_device;
        TempDescriptorSet m_tempDescriptorSet;
    };
} // namespace nitro::rhi::metal
