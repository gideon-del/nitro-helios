#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>

namespace nitro::rhi::metal
{
    MetalDescriptorLayout::MetalDescriptorLayout(MetalDevice *device, std::vector<RHIDescriptorBinding> bindings) : m_bindings(bindings),
                                                                                                                    m_device(device) {}
    MetalDescriptorLayout::~MetalDescriptorLayout() {}
} // namespace nitro::rhi::metal
