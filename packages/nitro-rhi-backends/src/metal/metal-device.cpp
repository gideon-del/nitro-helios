#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>
#include <nitro-rhi-backends/metal/metal-descriptor-set.h>
#include <nitro-rhi-backends/metal/metal-render-pass.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>
#include <nitro-rhi-backends/metal/metal-timer.h>
#include <nitro-rhi-backends/metal/metal-compute-pipeline.h>
#include <nitro-rhi-backends/metal/metal-heap.h>
#include <imgui_impl_glfw.h>
#ifndef IMGUI_IMPL_METAL_CPP
#define IMGUI_IMPL_METAL_CPP
#endif
#include <imgui_impl_metal.h>
#include <imnodes.h>
#include <GLFW/glfw3.h>
#include "ImGuizmo.h"
namespace nitro::rhi::metal
{
    MetalDevice::MetalDevice(void *window) : m_window(window)
    {
        device = MTL::CreateSystemDefaultDevice();
        commandQueue = device->newCommandQueue();

        m_frameEvent = device->newSharedEvent();

        m_defaultSamplers.anisotropicRepeat = create(RHISamplerDesc::AnisotropicRepeat());
        m_defaultSamplers.linearClamp = create(RHISamplerDesc::LinearClamp());
        m_defaultSamplers.linearRepeat = create(RHISamplerDesc::LinearRepeat());
        m_defaultSamplers.nearestClamp = create(RHISamplerDesc::NearestClamp());
        m_defaultSamplers.nearestRepeat = create(RHISamplerDesc::NearestRepeat());
        m_defaultSamplers.shadow = create(RHISamplerDesc::Shadow());

        float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;

        GLFWwindow *glfwWindow = reinterpret_cast<GLFWwindow *>(window);
        ImGui_ImplGlfw_InitForOther(glfwWindow, true);
        ImGui_ImplMetal_Init(device);
        ImNodes::CreateContext();
    }
    MetalDevice::~MetalDevice()
    {
        waitIdle();

        m_texturePool.clear();
        m_textureGraveyard.collectAll();

        m_bufferPool.clear();
        m_bufferGraveyard.collectAll();

        m_heapPool.clear();
        m_heapGraveyard.collectAll();

        ImNodes::DestroyContext();
        ImGui_ImplMetal_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        commandQueue->release();
        m_frameEvent->release();

        device->release();
    }

    RHIBufferHandle MetalDevice::create(const BufferDesc &desc)
    {
        auto handle = m_bufferPool.emplace(std::make_unique<MetalBuffer>(this, desc));
        auto buffer = m_bufferPool.get(handle)->get();

        assignHandle(*buffer, handle);

        return handle;
    }
    RHIBuffer *MetalDevice::resolve(const RHIBufferHandle &handle)
    {
        auto buffer = m_bufferPool.get(handle);

        if (!buffer)
        {
            return nullptr;
        }

        return buffer->get();
    }
    RHIBuffer *MetalDevice::createBuffer(const BufferDesc &desc)
    {
        return resolve(create(desc));
    }

    RHITextureHandle MetalDevice::create(const TextureDesc &desc)
    {
        auto handle = m_texturePool.emplace(std::make_unique<MetalTexture>(this, desc));
        auto texture = m_texturePool.get(handle)->get();

        assignHandle(*texture, handle);

        return handle;
    }

    RHITexture *MetalDevice::resolve(const RHITextureHandle &handle)
    {
        auto texture = m_texturePool.get(handle);

        if (!texture)
        {
            return nullptr;
        }

        return texture->get();
    }
    RHITexture *MetalDevice::createTexture(const TextureDesc &desc)
    {
        return resolve(create(desc));
    }
    RHIDescriptorLayout *MetalDevice::createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings)
    {
        return new MetalDescriptorLayout(this, bindings);
    }
    void MetalDevice::destroyDescriptorLayout(RHIDescriptorLayout *layout)
    {
        delete layout;
    };
    RHIDescriptorSet *MetalDevice::createDescriptorSet(RHIDescriptorLayout *layout)
    {
        MetalDescriptorLayout *metalLayout = reinterpret_cast<MetalDescriptorLayout *>(layout);
        return new MetalDescriptorSet(this, metalLayout);
    }
    void MetalDevice::destroyDescriptorSet(RHIDescriptorSet *set)
    {
        delete set;
    }
    RHIRenderPass *MetalDevice::createRenderPass(const RenderPassDesc &desc)
    {
        return new MetalRenderPass(this, desc);
    };
    void MetalDevice::destroyRenderPass(RHIRenderPass *renderPass)
    {
        delete renderPass;
    }
    RHIPipeline *MetalDevice::createPipeline(const PipelineDesc &desc)
    {
        return new MetalPipeline(this, desc);
    }

    RHIComputePipeline *MetalDevice::createComputePipeline(const ComputePipelineDesc &desc)
    {
        return new MetalComputePipeline(this, desc);
    }

    void MetalDevice::destroyComputePipeline(RHIComputePipeline *pipeline)
    {
        delete pipeline;
    }
    RHITimer *MetalDevice::createTimer()
    {
        return new MetalTimer(this);
    }

    void MetalDevice::destroy(const RHIBufferHandle &handle)
    {
        auto buffer = m_bufferPool.get(handle);
        if (!buffer)
            return;
        m_bufferGraveyard.push(std::move(*buffer), retireAt());

        m_bufferPool.free(handle);
    }
    void MetalDevice::destroyBuffer(RHIBuffer *buffer)
    {
        if (!buffer)
            return;
        destroy(buffer->getHandle());
    }

    void MetalDevice::destroy(const RHITextureHandle &handle)
    {
        auto texture = m_texturePool.get(handle);

        if (!texture)
        {
            return;
        }

        m_textureGraveyard.push(std::move(*texture), retireAt());
        m_texturePool.free(handle);
    }

    void MetalDevice::destroyTexture(RHITexture *texture)
    {
        if (!texture)
        {
            return;
        }

        destroy(texture->getHandle());
    }
    void MetalDevice::destroyPipeline(RHIPipeline *pipeline)
    {
        delete pipeline;
    }

    RHISwapchain *MetalDevice::createSwapchain(RHISurface *surface)
    {
        MetalSwapchain *swapchain = new MetalSwapchain(this, m_window);

        m_swapchain = swapchain;

        return swapchain;
    }
    RHICommandBuffer *MetalDevice::beginFrame()
    {
        waitForFrame();
        if (!m_swapchain)
        {
            throw std::runtime_error("Metal Swapchain not found");
        }
        if (m_swapchain->currentDrawable)
        {
            m_swapchain->currentDrawable->release();
        }
        m_swapchain->currentDrawable = m_swapchain->layer->nextDrawable();

        collectGarbage();
        m_currentCommandBuffer = new MetalCommandBuffer(this, m_swapchain);
        return m_currentCommandBuffer;
    }

    void MetalDevice::collectGarbage()
    {

        auto completed = getFrameCompleted();

        m_textureGraveyard.collect(completed);
        m_bufferGraveyard.collect(completed);
        m_heapGraveyard.collect(completed);
    }
    RHICommandBuffer *MetalDevice::createCommandBuffer()
    {

        m_currentCommandBuffer = new MetalCommandBuffer(this);
        return m_currentCommandBuffer;
    }

    RHISamplerHandle MetalDevice::create(const RHISamplerDesc &desc)
    {
        return RHISamplerHandle{};
    };
    void MetalDevice::destroy(RHISamplerHandle &handle)
    {
    }
    void MetalDevice::endFrame(RHICommandBuffer *cmd)
    {
        MetalCommandBuffer *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);
        metalCmd->commandBuffer->encodeSignalEvent(m_frameEvent, m_frameNumber + 1);
        metalCmd->commandBuffer->commit();

        m_frameNumber++;
        delete cmd;
    }
    void MetalDevice::endCommandBuffer(RHICommandBuffer *cmd)
    {
        MetalCommandBuffer *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);
        metalCmd->endEncoders();
        metalCmd->commandBuffer->commit();
        metalCmd->commandBuffer->waitUntilCompleted();
        delete cmd;
    }
    void MetalDevice::waitIdle()
    {
        m_frameEvent->waitUntilSignaledValue(m_frameNumber, UINT64_MAX);
    };

    uint32_t MetalDevice::getCurrentFrameIndex() const
    {
        return m_frameNumber % kMaxFramesInFlight;
    }
    void MetalDevice::beginImGuiFrame()
    {
        if (!m_currentCommandBuffer)
        {
            throw std::runtime_error("Must begin frame first befor imgui frame");
        }
        if (!m_currentCommandBuffer->rpd)
        {
            throw std::runtime_error("Must call in main render pass");
        }
        ImGui_ImplMetal_NewFrame(m_currentCommandBuffer->rpd);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }
    void MetalDevice::endImGuiFrame()
    {
        ImGui::Render();
    }
    void MetalDevice::drawImGui(RHICommandBuffer *cmd)
    {
        MetalCommandBuffer *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);

        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), metalCmd->commandBuffer, metalCmd->encoder);
    }

    void *MetalDevice::getImGuiTextureRef(RHITexture *texture)
    {
        auto *mtlTex = static_cast<MetalTexture *>(texture);
        return (void *)mtlTex->texture;
    }

    MemoryRequirements MetalDevice::textureMemoryRequirements(const TextureDesc &desc)
    {
        MTL::TextureDescriptor *textureDesc = makeMTLTextureDescriptor(desc);
        MTL::SizeAndAlign sizeAndAlign = device->heapTextureSizeAndAlign(textureDesc);

        size_t size = sizeAndAlign.size;
        size_t alignment = sizeAndAlign.align;

        textureDesc->release();

        return MemoryRequirements{
            size,
            alignment, 0};
    }

    RHIHeap *MetalDevice::createHeap(size_t sizeBytes, uint32_t memoryTypeBits)
    {
        MTL::HeapDescriptor *heapDesc = MTL::HeapDescriptor::alloc()->init();
        heapDesc->setSize(sizeBytes);
        heapDesc->setStorageMode(MTL::StorageModePrivate);

        auto handle = m_heapPool.emplace(std::make_unique<MetalHeap>());
        auto heap = m_heapPool.get(handle)->get();

        assignHandle(*heap, handle);
        heap->heap = device->newHeap(heapDesc);
        heapDesc->release();
        return heap;
    }

    void MetalDevice::destroyHeap(RHIHeap *heap)
    {
        if (!heap)
            return;
        auto handle = heap->getHandle();
        auto heapResource = m_heapPool.get(handle);

        if (!heapResource)
            return;
        m_heapGraveyard.push(std::move(*heapResource), retireAt());
        m_heapPool.free(handle);
    }

    RHITexture *MetalDevice::createTextureFromHeap(RHIHeap *heap, const TextureDesc &desc, size_t offset)
    {
        auto *mtlHeap = static_cast<MetalHeap *>(heap);
        MTL::TextureDescriptor *textureDesc = makeMTLTextureDescriptor(desc); // your shared desc-building function
        MTL::Texture *texture = mtlHeap->heap->newTexture(textureDesc, offset);
        auto handle = m_texturePool.emplace(std::make_unique<MetalTexture>(this, texture, desc));
        auto mtlTexture = m_texturePool.get(handle)->get();

        assignHandle(*mtlTexture, handle);
        textureDesc->release();

        return mtlTexture;
    }
} // namespace nitro::rhi::metal
