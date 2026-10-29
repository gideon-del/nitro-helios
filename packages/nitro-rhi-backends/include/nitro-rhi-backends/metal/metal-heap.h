#pragma once
#include <Metal/Metal.hpp>
#include <nitro-rhi/rhi-heap.h>

namespace nitro::rhi::metal
{
    class MetalHeap : public RHIHeap
    {
    public:
        MetalHeap() {};
        ~MetalHeap()
        {
            if (heap)
            {
                heap->release();
            }
        }
        MTL::Heap *heap = nullptr;
    };
} // namespace nitro::rhi::metal
