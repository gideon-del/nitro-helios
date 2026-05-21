#pragma once
#include <vulkan/vulkan.h>
#include <iostream>
void inline checkVkResult(const VkResult result, std::string message = "Something went wrong")
{

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Vulkan Error: " + std::to_string(result) + " " + message);
    };
}