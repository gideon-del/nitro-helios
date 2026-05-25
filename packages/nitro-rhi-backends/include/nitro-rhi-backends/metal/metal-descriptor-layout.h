#pragma once
#include <nitro-rhi/rhi-descriptor-layout.h>
#include <vector>
namespace nitro::rhi::metal
{
    class MetalDevice;
    class MetalDescriptorLayout : public RHIDescriptorLayout
    {
    public:
        MetalDescriptorLayout(MetalDevice *device, std::vector<RHIDescriptorBinding> bindings);
        ~MetalDescriptorLayout() override;

    private:
        MetalDevice *m_device;
        std::vector<RHIDescriptorBinding> m_bindings;
    };
} // namespace nitro::rhi::metal
