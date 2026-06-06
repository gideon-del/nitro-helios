#include <nitro-renderer/passes/shadow-pass.h>

namespace nitro::renderer
{
    ShadowPass::ShadowPass(rhi::RHIDevice *device, int cascadeIndex) : cascadeIndex(cascadeIndex)
    {
        rhi::TextureDesc shadowDesc{};
        shadowDesc.size.width = ShadowPass::c_ShadowResolution;
        shadowDesc.size.height = ShadowPass::c_ShadowResolution;
        shadowDesc.format = rhi::TextureDesc::ImageFormat::Depth32Float;
        shadowDesc.usage = rhi::TextureDesc::Usage::DepthStencil | rhi::TextureDesc::Usage::ShaderRead;
        shadowDesc.sampler = rhi::TextureDesc::Sampler::Depth;

        shadowTexture = device->createTexture(shadowDesc);

        rhi::RenderPassDesc shadowRenderPassDesc;
        rhi::RenderPassDesc::Attachment shadowDepthAttachment;
        shadowDepthAttachment.texture = shadowTexture;
        shadowDepthAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        shadowRenderPassDesc.depthAttachment = &shadowDepthAttachment;
        shadowRenderPassDesc.width = ShadowPass::c_ShadowResolution;
        shadowRenderPassDesc.height = ShadowPass::c_ShadowResolution;

        m_renderPass = device->createRenderPass(shadowRenderPassDesc);
    }
    void ShadowPass::execute(rhi::RHICommandBuffer *cmd, rhi::RHIPipeline *pipeline, rhi::RHIDescriptorSet *descriptorSet, Scene &scene)
    {

        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(pipeline);
        cmd->bindDescriptorSet(descriptorSet, 0);
        for (auto &obj : scene.objects)
        {
            ShadowPushConstant shadowPc;
            shadowPc.model = obj.transformation.getTransform().model;
            shadowPc.cascadeIndex = cascadeIndex;
            obj.draw(cmd, &shadowPc, sizeof(ShadowPushConstant));
        }
        cmd->endRenderPass();
    };
    float ShadowPass::s_getUniformSplit(
        float nearPlane,
        float farPlane,
        uint32_t cascadeCount,
        uint32_t cascadeIndex)
    {
        return nearPlane +
               (farPlane - nearPlane) *
                   (float(cascadeIndex) / float(cascadeCount));
    }
    float ShadowPass::s_getLogarithmSplit(
        float nearPlane,
        float farPlane,
        uint32_t cascadeCount,
        uint32_t cascadeIndex)
    {
        float ratio = farPlane / nearPlane;

        return nearPlane *
               std::pow(
                   ratio,
                   float(cascadeIndex) /
                       float(cascadeCount));
    }
    float ShadowPass::s_getPracticalSplit(
        float nearPlane,
        float farPlane,
        uint32_t cascadeCount,
        uint32_t cascadeIndex,
        float lambda)
    {
        return lambda * ShadowPass::s_getLogarithmSplit(nearPlane, farPlane, cascadeCount, cascadeIndex) +
               (1.0f - lambda) * ShadowPass::s_getUniformSplit(nearPlane, farPlane, cascadeCount, cascadeIndex);
    }
    constexpr float zMult = 10.0f;

    glm::vec3 calculateCornerPos(float halfHeight, float halfWidth, glm::vec3 depthPos, glm::vec3 upPos, glm::vec3 rightPos, glm::vec3 cameraPos)
    {

        return cameraPos + (upPos * halfHeight) + (rightPos * halfWidth) + depthPos;
    }

    glm::mat4 ShadowPass::s_calculateLightOrthoProj(
        float nearPlane,
        float farPlane,
        uint32_t cascadeCount,
        uint32_t cascadeIndex,
        float fov,
        float aspect,
        glm::mat4 cameraView,
        glm::vec3 cameraPos,
        glm::mat4 lightView)
    {
        float currentNear = cascadeIndex == 0 ? nearPlane : ShadowPass::s_getPracticalSplit(nearPlane, farPlane, cascadeCount, cascadeIndex);
        float currentFar = cascadeIndex == cascadeCount - 1 ? farPlane : ShadowPass::s_getPracticalSplit(nearPlane, farPlane, cascadeCount, cascadeIndex + 1);

        glm::mat4 invView = glm::inverse(cameraView);
        glm::vec3 right = glm::normalize(glm::vec3(invView[0]));
        glm::vec3 up = glm::normalize(glm::vec3(invView[1]));
        glm::vec3 forward = -glm::normalize(glm::vec3(invView[2]));

        glm::vec3 nearDepthForward = forward * currentNear;
        glm::vec3 farDepthForward = forward * currentFar;

        float nearHalfHeight = std::tan(fov / 2.0) * currentNear;
        float nearHalfWidth = aspect * nearHalfHeight;

        float farHalfHeight = std::tan(fov / 2.0) * currentFar;
        float farHalfWidth = aspect * farHalfHeight;

        std::vector<glm::vec4> corners;

        for (int x = 0; x < 2; x++)
        {
            float xSide = x == 0 ? -1 : 1;
            for (int y = 0; y < 2; y++)
            {
                float ySide = y == 0 ? -1 : 1;

                // glm::vec4 cornerF = glm::vec4(calculateCornerPos(ySide * farHalfHeight, xSide * farHalfWidth, farDepthForward, up, right, cameraPos), 1.0f);
                glm::vec4 cornerF = invView * glm::vec4(xSide * farHalfWidth, ySide * farHalfHeight, -currentFar, 1.0f);

                // glm::vec4 cornerN = glm::vec4(calculateCornerPos(ySide * nearHalfHeight, xSide * nearHalfWidth, nearDepthForward, up, right, cameraPos), 1.0f);
                glm::vec4 cornerN = invView * glm::vec4(xSide * nearHalfWidth, ySide * nearHalfHeight, -currentNear, 1.0f);
                corners.push_back(lightView * cornerN);
                corners.push_back(lightView * cornerF);
            };
        }

        glm::vec4 firstCorner = corners[0];

        float minX = firstCorner.x;
        float minY = firstCorner.y;
        float minZ = firstCorner.z;
        float maxX = firstCorner.x;
        float maxY = firstCorner.y;
        float maxZ = firstCorner.z;

        for (auto &corner : corners)
        {
            minX = std::min(minX, corner.x);
            maxX = std::max(maxX, corner.x);
            minY = std::min(minY, corner.y);
            maxY = std::max(maxY, corner.y);
            minZ = std::min(minZ, corner.z);
            maxZ = std::max(maxZ, corner.z);
        }

        // Texture snapping

        float centerX = (minX + maxX) * 0.5f;
        float centerY = (minY + maxY) * 0.5f;

        float width = maxX - minX;
        float height = maxY - minY;

        float texelX = width / ShadowPass::c_ShadowResolution;
        float texelY = height / ShadowPass::c_ShadowResolution;

        centerX = std::round(centerX / texelX) * texelX;
        centerY = std::round(centerY / texelY) * texelY;

        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;

        minX = centerX - halfWidth;
        maxX = centerX + halfWidth;

        minY = centerY - halfHeight;
        maxY = centerY + halfHeight;

        // if (maxZ > 0)
        // {
        //     minZ -= maxZ;
        //     maxZ = -0.01f;
        // }
        float zPaddingFactor = 0.75f;
        float depthRange = maxZ - minZ;
        float zPadding = depthRange * zPaddingFactor;

        minZ -= zPadding;
        maxZ += zPadding;

        auto projB =
            glm::orthoRH_ZO(
                minX, maxX,
                minY, maxY,
                -maxZ, -minZ);
        return projB *
               lightView;
    }

} // namespace nitro::renderer
