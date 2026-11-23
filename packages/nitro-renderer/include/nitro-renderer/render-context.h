#pragma once
#include <glm/glm.hpp>
#include "nitro-rhi/rhi-buffer.h"
#include "nitro-rhi/rhi-texture.h"
#include "nitro-rhi/pipeline-cache.h"
#include "nitro-rhi/descriptor-layout-cache.h"
#include "nitro-frame/graph/render-graph.h"
#include "nitro-frame/view/render-view.h"
#include <span>
#include "nitro-renderer/shader-locator.h"
namespace nitro::renderer
{

    struct EnvironmentData
    {

        rhi::RHITextureHandle environment, irradiance, prefiltered, brdfLut;
        uint32_t prefilterMipCount = 0;
        float intensity = 1.0f;

        glm::vec3 ambientColor{1.0f};
        float ambientIntensity = 0.03f;
        glm::vec3 skyColor{0.4f, 0.6f, 0.9f};

        bool valid() const { return environment.isValid(); }
    };

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
        rhi::RHIDescriptorLayout *materialTextureLayout = nullptr;
        rhi::RHIDescriptorSet *materialTextureSet = nullptr;

        DirectionalLight directionalLight;

        rhi::RHIBufferHandle lightBuffer;
        size_t lightCount = 0;

        EnvironmentData environment;
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

        enum class DebugMode : uint32_t
        {
            Lit,
            Depth,
            Albedo,
            Normal,
            Material,
            Emissive,
            WorldPosition,

            GGX,
            Fresnel,
            Geometry,
            Ambient,
            Direct,
            DiffuseIBL,
            SpecularIBL
        } debugMode = DebugMode::Lit;

        struct Lighting
        {
            float ambient = 0.3f;
        } lighting;

        bool lodEnabled = true;
        bool frustumCullEnabled = true;
        bool occlusionCullEnabled = false;
    };

    struct PostSettings
    {
        struct Bloom
        {
            float threshold = 0.4f;
            float intensity = 0.5f;
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

    struct SceneGraphResources
    {
        frame::LogicalBufferHandle vertices;
        frame::LogicalBufferHandle indices;
        frame::LogicalBufferHandle meshes;
        frame::LogicalBufferHandle instances;
        frame::LogicalBufferHandle sceneInstanceIds;
        frame::LogicalBufferHandle materials;
        frame::LogicalBufferHandle lights;

        frame::LogicalTextureHandle environment;
        frame::LogicalTextureHandle irradiance;
        frame::LogicalTextureHandle prefilter;
        frame::LogicalTextureHandle brdfLUT;
    };

    struct RenderContext
    {
        frame::RenderGraphBuilder &builder;
        const frame::RenderView &view;
        const RenderRequest &request;
        rhi::PipelineCache &pipelines;
        rhi::DescriptorLayoutCache &layouts;
        const ShaderLocator &shaders;
        const SceneGraphResources &scene;
        const rhi::DefaultSamplers &samplers;
    };

} // namespace nitro::renderer
