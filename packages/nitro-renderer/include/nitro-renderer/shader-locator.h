#pragma once
#include "nitro-rhi/rhi-pipeline.h"
#include <string_view>
namespace nitro::renderer
{
    // nitro-renderer
    class ShaderLocator
    {
    public:
        ShaderLocator(std::string shaderDir, bool isMetal) : m_dir(std::move(shaderDir)), m_isMetal(isMetal) {}

        rhi::ShaderDesc compute(std::string_view name) const;
        rhi::ShaderDesc vertex(std::string_view name) const;
        rhi::ShaderDesc fragment(std::string_view name) const;

    private:
        std::string m_dir;
        bool m_isMetal;
    };
} // namespace nitro::renderer
