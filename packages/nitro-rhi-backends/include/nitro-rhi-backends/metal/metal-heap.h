#pragma once
#include <Metal/Metal.hpp>
#include <nitro-rhi/rhi-heap.h>

namespace nitro::rhi::metal
{
    class MetalHeap : public RHIHeap
    {
    public:
        MTL::Heap *heap;
    };
} // namespace nitro::rhi::metal
