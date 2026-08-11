#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{
    class MeshCompactPass
    {
    public:
        MeshCompactPass(std::shared_ptr<rhi::RHIDevice> device, std::string shaderDir, bool isMetal);
        ~MeshCompactPass();

    private:
    };
} // namespace nitro::renderer
