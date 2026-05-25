# Nitro Helios

## RHI v0.0.1

### Added RHI abstractions

Added all abstraction layers in `packages/nitro-rhi/include`

1. `RHIBuffer`: This is the abstractions responsible for platform buffers (`VKBuffer` & `MTLBuffer`).
2. `RHITexture`: This is the abstraction for all texture related resource.
3. `RHICommandBuffer`: This contains the command buffer interface for each backend (`VKCommandBuffer` & `MTLCommandBuffer`).
4. `RHIDescriptorLayout`: This abstraction was created mostly for vulkan to generate the `VkDescriptorSetLayout` and `VkDescriptorPool` for Uniform Buffers and Samplers. There's a `RHIDescriptorBinding` for this specific reason.
5. `RHIDescriptorSet`: This interface ensures each backend handles the binding to uniform buffers and samplers to the shader.
6. `RHISurface`: This abstractions is for the window surface binding. For now it's used in the vulkan backend to attach the `VkSurfaceKHR` to the `GLFWwindow`
7. `RHISwapchain`: This abstraction provides a way to get the current back buffer that for presentation.
8. `RHIPipeline`: This is the abstraction for pipelines on each backend. It is constructed with the use of the `PipelineDesc` struct. The struct also contains the `RHIDescriptorLayout`. This is used to add the descriptorSetLayout to the Vulkan Pipeline Layout 9.`RHIDevice`: This is the abstraction for each backend device. It also has interfaces for creating and destroying resources like buffers, textures, pipelines, command buffer and so on. The device also has a `getCurrentFrameIndex` method. This was created for vulkan in order to find a way not to create stale data while reading and updating the same resource.

### Vulkan Backend

#### VulkanBuffer

The `VulkanBuffer` is the implementation of the `RHIBuffer`. It contains the `VkBuffer` and `VmaAllocation`.
The constructor takes in the `VulkanDevice` and the `BufferDesc` struct. I decided to add the `initialData` pointer to the desc in the case of creating vertex or index buffers, so that I can create a staging buffer to upload the data and copy to the main buffer.
The buffer also has an `upload` method. Useful for uploading data mid frame.

#### VulkanTexture

The `VulkanTexture` is the implementation for the `RHITexture`. It contains the `VkImage`, `VmaAllocation`, `VkImageView`, `VkSampler` (for shader read usage only) and a pointer to the `VulkanDevice`.

_Remaining backend classes documented in code. See packages/nitro-rhi-backends/._
