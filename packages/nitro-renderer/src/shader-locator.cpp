#include "nitro-renderer/shader-locator.h"

namespace nitro::renderer
{
    rhi::ShaderDesc ShaderLocator::compute(std::string_view name) const
    {
        const std::string n{name};
        const std::string base = m_dir + "/" + n + "/" + n;

        rhi::ShaderDesc desc;
        desc.stage = rhi::ShaderStage::Compute;
        if (m_isMetal)
        {
            desc.name = "comp";
            desc.filePath = base + ".metallib";
        }
        else
        {
            desc.name = "main";
            desc.filePath = base + ".comp.spv";
        }
        return desc;
    }
    rhi::ShaderDesc ShaderLocator::fragment(std::string_view name) const
    {
        const std::string n{name};
        const std::string base = m_dir + "/" + n + "/" + n;

        rhi::ShaderDesc desc;
        desc.stage = rhi::ShaderStage::Fragment;
        if (m_isMetal)
        {
            desc.name = "fs";
            desc.filePath = base + ".metallib";
        }
        else
        {
            desc.name = "main";
            desc.filePath = base + ".frag.spv";
        }
        return desc;
    }
    rhi::ShaderDesc ShaderLocator::vertex(std::string_view name) const
    {
        const std::string n{name};
        const std::string base = m_dir + "/" + n + "/" + n;

        rhi::ShaderDesc desc;
        desc.stage = rhi::ShaderStage::Vertex;
        if (m_isMetal)
        {
            desc.name = "vs";
            desc.filePath = base + ".metallib";
        }
        else
        {
            desc.name = "main";
            desc.filePath = base + ".vert.spv";
        }
        return desc;
    }
} // namespace nitro::renderer
