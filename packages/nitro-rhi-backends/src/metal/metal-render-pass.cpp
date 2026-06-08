#include <nitro-rhi-backends/metal/metal-render-pass.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
#include <nitro-rhi-backends/metal/metal-device.h>

namespace nitro::rhi::metal
{

    MTL::LoadAction convertToLoadAction(RenderPassDesc::LoadOp load)
    {
        switch (load)
        {
        case RenderPassDesc::LoadOp::Clear:
            return MTL::LoadActionClear;
        case RenderPassDesc::LoadOp::DontCare:
            return MTL::LoadActionDontCare;
        case RenderPassDesc::LoadOp::Load:
            return MTL::LoadActionLoad;

        default:
            return MTL::LoadActionDontCare;
        }
    }
    MTL::StoreAction convertToStoreAction(RenderPassDesc::StoreOp store)
    {
        switch (store)
        {
        case RenderPassDesc::StoreOp::DontCare:
            return MTL::StoreActionDontCare;
        case RenderPassDesc::StoreOp::Store:
            return MTL::StoreActionStore;
        default:
            return MTL::StoreActionDontCare;
        }
    }
    MetalRenderPass::MetalRenderPass(MetalDevice *device, const RenderPassDesc &desc) : m_device(device), width(desc.width), height(desc.height)
    {
        renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
        if (desc.depthAttachment)
        {
            depthTexture = reinterpret_cast<MetalTexture *>(desc.depthAttachment->texture);
            renderPassDescriptor->depthAttachment()->setClearDepth(NS::UInteger(desc.depthAttachment->clearDepth));
            renderPassDescriptor->depthAttachment()->setLoadAction(convertToLoadAction(desc.depthAttachment->load));
            renderPassDescriptor->depthAttachment()->setStoreAction(convertToStoreAction(desc.depthAttachment->store));
            renderPassDescriptor->depthAttachment()->setTexture(depthTexture->texture);
        }

        if (!desc.colorAttachments.empty())
        {
            for (int i = 0; i < desc.colorAttachments.size(); i++)
            {
                auto &currentColorAttachment = desc.colorAttachments[i];
                MetalTexture *colorTexture = reinterpret_cast<MetalTexture *>(currentColorAttachment.texture);
                renderPassDescriptor->colorAttachments()->object(i)->setClearColor({currentColorAttachment.clearColor[0],
                                                                                    currentColorAttachment.clearColor[1],
                                                                                    currentColorAttachment.clearColor[2],
                                                                                    currentColorAttachment.clearColor[3]});
                renderPassDescriptor->colorAttachments()->object(i)->setTexture(colorTexture->texture);
                renderPassDescriptor->colorAttachments()->object(i)->setStoreAction(convertToStoreAction(currentColorAttachment.store));
                renderPassDescriptor->colorAttachments()->object(i)->setLoadAction(convertToLoadAction(currentColorAttachment.load));
            }
        }
    }
    MetalRenderPass::~MetalRenderPass()
    {

        // if (renderPassDescriptor != nullptr)
        // {
        //     renderPassDescriptor->release();
        // }
    }
} // namespace nitro::rhi::metal
