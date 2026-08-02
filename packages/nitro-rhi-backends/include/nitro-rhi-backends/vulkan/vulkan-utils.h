#pragma once
#include <vulkan/vulkan.h>
#include <iostream>

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

} // namespace nitro::rhi::vulkan
