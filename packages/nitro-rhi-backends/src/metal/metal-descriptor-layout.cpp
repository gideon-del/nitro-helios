#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>

namespace nitro::rhi::metal
{
    MetalDescriptorLayout::MetalDescriptorLayout(MetalDevice *device, std::vector<RHIDescriptorBinding> bindings) : m_bindings(bindings),
                                                                                                                    m_device(device)
    {
        for (auto &binding : bindings)
        {
            if (binding.type == RHIDescriptorBinding::Type::UniformBuffer)
            {
                bufferBindings[binding.binding] = binding.stage;
            }
            if (binding.type == RHIDescriptorBinding::Type::Sampler)
            {
                textureBindings[binding.binding] = binding.stage;
            }
        }
    }
    MetalDescriptorLayout::~MetalDescriptorLayout() {}
} // namespace nitro::rhi::metal
