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
    MetalRenderPass::MetalRenderPass(MetalDevice *device, const RenderPassDesc &desc) : m_device(device)
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

        if (desc.colorAttachment)
        {
            colorTexture = reinterpret_cast<MetalTexture *>(desc.colorAttachment->texture);
            renderPassDescriptor->colorAttachments()->object(0)->setClearColor({desc.colorAttachment->clearColor[0],
                                                                                desc.colorAttachment->clearColor[1],
                                                                                desc.colorAttachment->clearColor[2],
                                                                                desc.colorAttachment->clearColor[3]});
            renderPassDescriptor->colorAttachments()->object(0)->setTexture(colorTexture->texture);
            renderPassDescriptor->colorAttachments()->object(0)->setStoreAction(convertToStoreAction(desc.colorAttachment->store));
            renderPassDescriptor->colorAttachments()->object(0)->setLoadAction(convertToLoadAction(desc.colorAttachment->load));
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
