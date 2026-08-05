#pragma once
#include <cstddef>
#include <iostream>
#include "rhi-utils.h"
namespace nitro::rhi
{
    struct BufferDesc
    {

        enum class StorageMode
        {
            Shared,
            GPU
        } storage;

        enum class Usage
        {
            Vertex,
            Index,
            Uniform,
            Storage,
            Staging,
            TransferDst,
            Indirect
        } usage;

        size_t size;

        const void *initialData = nullptr;
    };

    class RHIBuffer
    {
    public:
        virtual ~RHIBuffer() = default;
        virtual void upload(const void *data, size_t size, size_t offset = 0) = 0;
        virtual size_t getSize() const = 0;

        virtual void *map() = 0;
        virtual void unmap() = 0;
    };

    struct BufferBarrier
    {
        RHIBuffer *buffer;
        ResourceState before;
        ResourceState after;
    };
}