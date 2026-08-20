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

        enum class Usage : uint32_t
        {
            Vertex = 1 << 0,
            Index = 1 << 1,
            Uniform = 1 << 2,
            Storage = 1 << 3,
            Staging = 1 << 4,
            TransferDst = 1 << 5,
            Indirect = 1 << 6
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

    inline BufferDesc::Usage operator|(BufferDesc::Usage a, BufferDesc::Usage b)
    {
        return static_cast<BufferDesc::Usage>(
            static_cast<uint32_t>(a) |
            static_cast<uint32_t>(b));
    };

    inline BufferDesc::Usage &operator|=(BufferDesc::Usage &a,
                                         BufferDesc::Usage b)
    {
        a = a | b;
        return a;
    }

    inline bool hasBufferUsageFlag(BufferDesc::Usage value,
                                   BufferDesc::Usage flag)
    {
        return (static_cast<uint32_t>(value) &
                static_cast<uint32_t>(flag)) != 0;
    }
}