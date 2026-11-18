#pragma once
#include <glm/glm.hpp>
#include "nitro-rhi/rhi-buffer.h"
#include "nitro-rhi/rhi-texture.h"
#include "nitro-frame/graph/render-graph.h"
#include "nitro-frame/view/render-view.h"
#include "nitro-frame/graph/blackboard.h"
#include <span>
namespace nitro::renderer
{

    struct CameraData
    {
        glm::mat4 view, proj, viewProj, invViewProj;
        glm::vec3 position;
        float farPlane, nearPlane;
        float fovY;
    };

    struct DirectionalLight
    {
        glm::mat4 view;
        glm::vec3 direction;
        glm::vec3 color = glm::vec3(1.0f);
        float intensity = 1.0f;
    };
    struct SceneRenderData
    {

        rhi::RHIBufferHandle vertexBuffer;
        rhi::RHIBufferHandle indexBuffer;
        rhi::RHIBufferHandle meshBuffer;
        rhi::RHIBufferHandle meshInstanceBuffer;
        size_t meshInstanceCount = 0;
        rhi::RHIBufferHandle sceneInstanceIdBuffer;

        std::span<const rhi::RHITextureHandle> materialTextures;
        rhi::RHIBufferHandle materialBuffer;

        DirectionalLight directionalLight;

        rhi::RHIBufferHandle lightBuffer;
        size_t lightCount = 0;
    };

    struct PipelineSettings
    {
        struct Shadow
        {
            float bias = 0.005f;
            float normalBias = 0.05f;
            float lambda = 0.5f;
            bool showCascadeColors = false;
        } shadow;

        struct SSAO
        {
            float radius = 0.2f;
            float depthSigma = 1.0f;
        } ssao;

        enum class DebugMode
        {
            Lit = 0,
            Albedo = 1,
            Normal = 2,
            Depth = 3,
            WorldPosition = 4,
            CascadeColor = 5,
            PointLight = 6,
            DirectionalLight = 7,
            HeatMap = 8,
            DistributionGGX = 9,
            FresnelSchlick = 10,
            GeometrySmith = 11,
            Ambient = 12,
            SpecularIBL = 13,
        } debugMode = DebugMode::Lit;

        struct Lighting
        {
            float ambient = 0.3f;
        } lighting;

        bool lodEnabled = true;
        bool frustumCullEnabled = true;
        bool occlusionCullEnabled = true;
    };

    struct PostSettings
    {
        struct Bloom
        {
            float threshold = 0.4;
            float intensity = 0.5;
            bool enable = false;
        } bloom;

        enum class ToneMapMode
        {
            Linear = 0,
            Reinhard = 1,
            ACES = 2,
        };
        struct ToneMap
        {
            float exposure = 0.4;
            ToneMapMode mode = ToneMapMode::ACES;
            bool autoExposure = false;
        } tonemap;

        struct ColorGrading
        {
            glm::vec3 lift = glm::vec3(0.0f);
            glm::vec3 gain = glm::vec3(1.0f);
            glm::vec3 gamma = glm::vec3(1.0f);
            bool enable = false;
        } colorGrading;
    };

    struct RenderRequest
    {
        CameraData camera{};

        PipelineSettings pipelineSettings;
        PostSettings postSettings;

        const SceneRenderData *sceneData = nullptr;

        float deltaTime = 0.0f;
        float time = 0.0f;
    };

    struct RenderContext
    {
        frame::RenderGraphBuilder &builder;
        const frame::RenderView &view;
        const RenderRequest &request;
    };
} // namespace nitro::renderer
