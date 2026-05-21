#include <nitro-rhi-backends/vulkan/vulkan-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>

namespace nitro::rhi::vulkan
{

    VmaAllocationCreateFlags convertToAllocationFlag(const BufferDesc::StorageMode &storage)
    {
        switch (storage)
        {
        case BufferDesc::StorageMode::GPU:
            return 0;
            break;
        case BufferDesc::StorageMode::Shared:
            return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        default:
            throw std::runtime_error("Invalid Storage Mode: ");
        };
    };
    VmaMemoryUsage convertToMemoryUsage(const BufferDesc::StorageMode &storage)
    {
        switch (storage)
        {
        case BufferDesc::StorageMode::GPU:
            return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case BufferDesc::StorageMode::Shared:
            return VMA_MEMORY_USAGE_AUTO;
            break;
        default:
            throw std::runtime_error("Invalid Storage Mode: ");
        };
    }
    VkBufferUsageFlags convertToBufferUsage(const BufferDesc::Usage &usage)
    {

        switch (usage)
        {
        case BufferDesc::Usage::Index:
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferDesc::Usage::Vertex:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferDesc::Usage::Uniform:
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case BufferDesc::Usage::Storage:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        case BufferDesc::Usage::Staging:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
    }
    VulkanBuffer::VulkanBuffer(VulkanDevice *device, const BufferDesc &desc) : m_device(device), m_size(desc.size)
    {
        VmaAllocationCreateInfo allocationInfo{};
        allocationInfo.flags = convertToAllocationFlag(desc.storage);
        allocationInfo.usage = convertToMemoryUsage(desc.storage);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.size = (VkDeviceSize)desc.size;
        bufferInfo.usage = convertToBufferUsage(desc.usage);

        checkVkResult(vmaCreateBuffer(
            m_device->allocator,
            &bufferInfo,
            &allocationInfo,
            &buffer,
            &allocation,
            nullptr));

        if (desc.storage == BufferDesc::StorageMode::GPU && desc.initialData != nullptr)
        {
            BufferDesc stagingDesc{};

            stagingDesc.initialData = nullptr;
            stagingDesc.size = desc.size;
            stagingDesc.storage = BufferDesc::StorageMode::Shared;
            stagingDesc.usage = BufferDesc::Usage::Staging;

            VulkanBuffer staging(device, stagingDesc);

            staging.upload(desc.initialData, desc.size);

            device->copyBuffer(staging.buffer, buffer, desc.size);
        };
    };

    void VulkanBuffer::upload(const void *data, size_t size)
    {
        void *bufferData;
        vmaMapMemory(m_device->allocator, allocation, &bufferData);
        memcpy(bufferData, data, size);
        vmaUnmapMemory(m_device->allocator, allocation);
    }

    size_t VulkanBuffer::getSize() const
    {
        return m_size;
    }
    VulkanBuffer::~VulkanBuffer()
    {
        if (buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_device->allocator, buffer, allocation);
        }
    }

}
