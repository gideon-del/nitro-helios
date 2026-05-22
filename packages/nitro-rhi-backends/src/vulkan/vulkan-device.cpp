#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-pipeline.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <vector>
#include <set>
namespace nitro::rhi::vulkan
{
    VkInstance create_instance()
    {
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        VkApplicationInfo appInfo{};
        const std::vector<const char *> validationLayers{
            "VK_LAYER_KHRONOS_validation"};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_3;
        appInfo.pEngineName = "My Engine";
        appInfo.pApplicationName = "Blank Window";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

        VkInstanceCreateInfo createInfo{};

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        createInfo.pApplicationInfo = &appInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        VkInstance instance;

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

        checkVkResult(result, "Failed to create instance:");

        return instance;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        std::cerr << "validation layer: type " << std::to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    VkDebugUtilsMessengerEXT inline attach_debugger(const VkInstance &instance)
    {
        VkDebugUtilsMessageSeverityFlagsEXT severityFlags =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

        VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags =
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback,

        };

        VkDebugUtilsMessengerEXT debugMessenger;
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

        if (func)
        {

            checkVkResult(func(instance, &debugUtilsMessengerCreateInfoEXT, nullptr, &debugMessenger), "Failed to create instance:");
        }
        else
        {
            throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT");
        }

        return debugMessenger;
    }

    VkPhysicalDevice inline select_physical_device(const VkInstance &instance)
    {

        uint32_t deviceCount;
        checkVkResult(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "Error fetching physical devices:");

        std::vector<VkPhysicalDevice> devices(deviceCount);

        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        if (devices.empty())
        {
            throw std::runtime_error("No Physical Device Found");
        }
        std::vector<VkPhysicalDevice> candidateDevices;

        std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        for (auto device : devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> allExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, allExtensions.data());

            std::set<std::string> requiredExtensions(
                deviceExtensions.begin(),
                deviceExtensions.end());

            for (auto extension : allExtensions)
            {

                requiredExtensions.erase(extension.extensionName);
            }

            if (requiredExtensions.empty())
            {

                candidateDevices.push_back(device);

                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    std::rotate(
                        candidateDevices.begin(),
                        candidateDevices.end() - 1,
                        candidateDevices.end());
                }
            }
        }

        if (candidateDevices.empty())
        {
            throw std::runtime_error("No gpu meets the minimum requirement");
        }

        VkPhysicalDevice device = candidateDevices[0];

        return device;
    }

    QueueFamilyIndices find_queue_families(const VkSurfaceKHR &surface, const VkPhysicalDevice &device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        QueueFamilyIndices indices;
        for (int i = 0; i < queueFamilies.size(); i++)
        {

            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupported = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupported);

            if (presentSupported)
            {
                indices.presentFamily = i;
            }
        }

        if (!indices.isComplete())
        {
            throw std::runtime_error("No Queue Family present that satisfies graphic and present");
        }

        return indices;
    }

    VkDevice create_logical_device(const VkPhysicalDevice &physicalDevice, const QueueFamilyIndices &queueIndices)
    {
        float queuePriorities = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{};

        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriorities;

        std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            "VK_KHR_portability_subset"};

        VkDeviceCreateInfo deviceInfo{};

        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceInfo.pEnabledFeatures = &deviceFeatures;

        VkDevice logicalDevice;
        checkVkResult(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &logicalDevice), "Logical Device not created");

        return logicalDevice;
    }

    VmaAllocator create_allocator(const VkInstance &instance, const VkPhysicalDevice &physicalDevice, const VkDevice &device)
    {
        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;

        VmaAllocator allocator;
        checkVkResult(vmaCreateAllocator(&allocatorCreateInfo, &allocator), "Allocator not created");

        return allocator;
    }

    VkCommandPool create_command_pool(const VkDevice &device, const QueueFamilyIndices &queueIndices)
    {
        VkCommandPoolCreateInfo commandPoolInfo{};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();

        VkCommandPool commandPool;
        checkVkResult(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool), "Command Pool not created");

        return commandPool;
    }
    VkFormat query_device_format(VkPhysicalDevice &physicalDevice, VkSurfaceKHR &surface)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        for (const auto &format : formats)
        {

            if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                return format.format;
            }
        }

        return formats[0].format;
    }
    VkRenderPass create_render_pass(const VkDevice &device, const VkFormat &format)
    {

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};

        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription{};

        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentRef;
        subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

        std::array<VkAttachmentDescription, 2> attachmentRefs = {colorAttachment, depthAttachment};
        VkSubpassDependency colorDependency{};
        colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        colorDependency.dstSubpass = 0;
        colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.srcAccessMask = 0;
        colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkSubpassDependency depthDependency{};
        depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depthDependency.dstSubpass = 0;
        depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        depthDependency.srcAccessMask = 0;
        depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkSubpassDependency, 2> dependency = {colorDependency, depthDependency};
        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = attachmentRefs.data();
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpassDescription;
        createInfo.dependencyCount = 2;
        createInfo.pDependencies = dependency.data();

        VkRenderPass renderPass;

        if (vkCreateRenderPass(device, &createInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Render Pass not created");
        }

        return renderPass;
    }

    VulkanDevice::VulkanDevice(void *window)
    {

        m_instance = create_instance();
        m_messageCallback = attach_debugger(m_instance);
        surface = new VulkanSurface(m_instance, window);
        m_physicalDevice = select_physical_device(m_instance);
        m_queueFamilyIndices = find_queue_families(surface->surface, m_physicalDevice);

        device = create_logical_device(m_physicalDevice, m_queueFamilyIndices);

        vkGetDeviceQueue(device, m_queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, m_queueFamilyIndices.presentFamily.value(), 0, &presentQueue);

        allocator = create_allocator(m_instance, m_physicalDevice, device);
        commandPool = create_command_pool(device, m_queueFamilyIndices);
        m_surfaceFormat = query_device_format(m_physicalDevice, surface->surface);
        defaultRenderPass = create_render_pass(device, m_surfaceFormat);
    }

    VulkanDevice::~VulkanDevice()
    {
        vkQueueWaitIdle(graphicsQueue);
        vkQueueWaitIdle(presentQueue);
        vkDestroyCommandPool(device, commandPool, nullptr);
        vmaDestroyAllocator(allocator);
        vkDestroyRenderPass(device, defaultRenderPass, nullptr);
        vkDestroyDevice(device, nullptr);
        delete surface;
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");

        if (func)
        {
            func(m_instance, m_messageCallback, nullptr);
        }

        vkDestroyInstance(m_instance, nullptr);
    }
    VkCommandBuffer VulkanDevice::beginOneTimeCommands()
    {
        VkCommandBufferAllocateInfo commanBufferAllocateInfo{};
        commanBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commanBufferAllocateInfo.commandBufferCount = 1;
        commanBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commanBufferAllocateInfo.commandPool = commandPool;

        VkCommandBuffer commandBuffer;

        checkVkResult(vkAllocateCommandBuffers(device, &commanBufferAllocateInfo, &commandBuffer),
                      "Can't create one-time command buffer");
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }
    void VulkanDevice::endOneTimeCommands(VkCommandBuffer &cmd)
    {

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        checkVkResult(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE),
                      "Failed to submit one-time command");
        vkQueueWaitIdle(graphicsQueue);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }
    void VulkanDevice::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
    {

        VkCommandBuffer commandBuffer = beginOneTimeCommands();

        VkBufferCopy bufferCopy{};
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;
        bufferCopy.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &bufferCopy);
        vkEndCommandBuffer(commandBuffer);

        endOneTimeCommands(commandBuffer);
    }

    RHIBuffer *VulkanDevice::createBuffer(const BufferDesc &desc)
    {
        return new VulkanBuffer(this, desc);
    }
    void VulkanDevice::destroyBuffer(RHIBuffer *buffer)
    {
        delete buffer;
    }

    RHITexture *VulkanDevice::createTexture(const TextureDesc &desc)
    {

        return new VulkanTexture(this, desc);
    }
    void VulkanDevice::destroyTexture(RHITexture *texture)
    {
        delete texture;
    }

    void VulkanDevice::transitionImageLayout(
        VkCommandBuffer &cmd,
        VkImage image,
        VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage,
        VkImageAspectFlags aspectMask)
    {
        VkImageMemoryBarrier imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.dstAccessMask = dstAccessMask;
        imageBarrier.image = image;
        imageBarrier.newLayout = newLayout;
        imageBarrier.oldLayout = oldLayout;
        imageBarrier.srcAccessMask = srcAccessMask;
        imageBarrier.subresourceRange.aspectMask = aspectMask;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.layerCount = 1;
        imageBarrier.subresourceRange.levelCount = 1;

        vkCmdPipelineBarrier(
            cmd,
            srcStage,
            dstStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageBarrier);
    }
    void VulkanDevice::copyBufferToImage(VkBuffer &src,
                                         VkImage &image,
                                         VkDeviceSize size,
                                         VkExtent3D imageExtent)
    {

        VkCommandBuffer commandBuffer = beginOneTimeCommands();

        transitionImageLayout(commandBuffer,
                              image,
                              0,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_IMAGE_ASPECT_COLOR_BIT);
        VkBufferImageCopy bufferImageCopy{};
        bufferImageCopy.bufferImageHeight = 0;
        bufferImageCopy.bufferRowLength = 0;
        bufferImageCopy.imageExtent = imageExtent;
        bufferImageCopy.bufferOffset = 0;
        bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferImageCopy.imageSubresource.baseArrayLayer = 0;
        bufferImageCopy.imageSubresource.layerCount = 1;
        bufferImageCopy.imageSubresource.mipLevel = 0;
        vkCmdCopyBufferToImage(
            commandBuffer,
            src,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &bufferImageCopy);

        transitionImageLayout(commandBuffer,
                              image,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_SHADER_READ_BIT,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                              VK_IMAGE_ASPECT_COLOR_BIT);
        endOneTimeCommands(commandBuffer);
    }

    RHIPipeline *VulkanDevice::createPipeline(const PipelineDesc &desc)
    {
        return new VulkanPipeline(this, desc);
    }

    void VulkanDevice::destroyPipeline(RHIPipeline *pipeline)
    {
        delete pipeline;
    }
}