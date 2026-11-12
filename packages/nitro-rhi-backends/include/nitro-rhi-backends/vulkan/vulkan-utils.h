#pragma once
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cassert>
#include "nitro-rhi/rhi-utils.h"
#include "nitro-rhi/rhi-texture.h"
#include "nitro-core/types/size.h"

namespace nitro::rhi::vulkan
{
    void inline checkVkResult(const VkResult result, std::string message = "Something went wrong")
    {

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Vulkan Error: " + std::to_string(result) + " " + message);
        };
    }

    uint32_t inline findVulkanMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            bool typeSupported = typeFilter & (1 << i);
            bool hasProperties = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;

            if (typeSupported && hasProperties)
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type");
    }

    struct VkStateInfo
    {
        VkPipelineStageFlags2 stage;
        VkAccessFlags2 access;
    };

    VkPipelineStageFlags2 inline convertResourceStateToStage(ResourceState state)
    {
        switch (state)
        {
        case ResourceState::Undefined:
            return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        case ResourceState::CopySrc:
        case ResourceState::CopyDst:
            return VK_PIPELINE_STAGE_2_TRANSFER_BIT;

        case ResourceState::ShaderRead:
            return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                   VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

        case ResourceState::ShaderWrite:
            return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

        case ResourceState::RenderTarget:
            return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        case ResourceState::DepthRead:
        case ResourceState::DepthReadStencilWrite:
            return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                   VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        case ResourceState::DepthWrite:
            return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                   VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

        case ResourceState::Present:
            return VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

        case ResourceState::IndirectDraw:
            return VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
        }
        assert(false && "unhandled ResourceState");
        return VK_PIPELINE_STAGE_2_NONE;
    }

    VkAccessFlags2 inline convertResourceStateToAccessMask2(ResourceState state)
    {
        switch (state)
        {
        case ResourceState::Undefined:
            return VK_ACCESS_2_NONE;

        case ResourceState::CopyDst:
            return VK_ACCESS_2_TRANSFER_WRITE_BIT;

        case ResourceState::CopySrc:
            return VK_ACCESS_2_TRANSFER_READ_BIT;

        case ResourceState::DepthRead:
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

        case ResourceState::DepthWrite:
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        case ResourceState::Present:
            return VK_ACCESS_2_NONE;

        case ResourceState::RenderTarget:
            return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

        case ResourceState::ShaderRead:
            return VK_ACCESS_2_SHADER_READ_BIT;

        case ResourceState::ShaderWrite:
            return VK_ACCESS_2_SHADER_WRITE_BIT;

        case ResourceState::DepthReadStencilWrite:
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                   VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                   VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        case ResourceState::IndirectDraw:
            return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        }

        assert(false && "unhandled ResourceState");
        return VK_ACCESS_2_NONE;
    }
    VkStateInfo inline toVkState(ResourceState s)
    {
        VkStateInfo info;
        info.stage = convertResourceStateToStage(s);
        info.access = convertResourceStateToAccessMask2(s);

        return info;
    }

    VkImageLayout inline convertResourceStateToImageLayout(ResourceState state)
    {
        switch (state)
        {
        case ResourceState::Undefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;

        case ResourceState::CopyDst:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        case ResourceState::CopySrc:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        case ResourceState::DepthRead:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        case ResourceState::DepthWrite:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        case ResourceState::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        case ResourceState::RenderTarget:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        case ResourceState::ShaderRead:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        case ResourceState::ShaderWrite:
            return VK_IMAGE_LAYOUT_GENERAL;

        case ResourceState::DepthReadStencilWrite:
            return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;

        case ResourceState::IndirectDraw:
            assert(false && "IndirectDraw is buffer-only");
            return VK_IMAGE_LAYOUT_UNDEFINED;

        default:
            assert(state != ResourceState::IndirectDraw && "Indirect draw are for buffers only");
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    VkExtent3D inline convertToVkExtent3D(const Extent3D &extent)
    {
        VkExtent3D vkExtent{};
        vkExtent.width = extent.width;
        vkExtent.height = extent.height;
        vkExtent.depth = extent.depth;

        return vkExtent;
    }
    VkOffset3D inline convertToVkOffset3D(const Offset3D &offset)
    {
        VkOffset3D vkOffset{};
        vkOffset.x = offset.x;
        vkOffset.y = offset.y;
        vkOffset.z = offset.z;

        return vkOffset;
    }

    VkImageAspectFlags inline aspectFromFormat(TextureDesc::ImageFormat f)
    {
        switch (f)
        {
        case TextureDesc::ImageFormat::Depth32Float:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case TextureDesc::ImageFormat::Depth32FloatStencil8:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    inline void setDebugName(VkDevice d, VkObjectType type, uint64_t handle, const char *name)
    {
#ifndef NDEBUG
        VkDebugUtilsObjectNameInfoEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.objectType = type;
        info.objectHandle = handle;
        info.pObjectName = name;
        vkSetDebugUtilsObjectNameEXT(d, &info);
#endif
    }
} // namespace nitro::rhi::vulkan
