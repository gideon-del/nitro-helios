#pragma once
#include "nitro-core/types/handle.h"

namespace nitro::rhi
{

    struct RHIHeapTag
    {
    };

    using RHIHeapHandle = Handle<RHIHeapTag>;
    using RHIHeapHandleHash = HandleHash<RHIHeapTag>;

    class RHIHeap
    {
        friend class RHIDevice;
        RHIHeapHandle m_handle;

    public:
        virtual ~RHIHeap() = default;

        RHIHeapHandle getHandle() const { return m_handle; }
    };

} // namespace nitro::rhi
