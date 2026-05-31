#pragma once
#include <nitro-rhi/rhi-descriptor-layout.h>
#include <vector>
#include <unordered_map>
namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalDescriptorLayout : public RHIDescriptorLayout
    {
    public:
        MetalDescriptorLayout(MetalDevice *device, std::vector<RHIDescriptorBinding> bindings);
        ~MetalDescriptorLayout() override;

        std::unordered_map<uint32_t, RHIDescriptorBinding::ShaderStage> bufferBindings;
        std::unordered_map<uint32_t, RHIDescriptorBinding::ShaderStage> textureBindings;

    private:
        MetalDevice *m_device;
        std::vector<RHIDescriptorBinding> m_bindings;
    };
} // namespace nitro::rhi::metal
