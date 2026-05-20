#pragma once
#include <cstddef>
#include <iostream>
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
            Storage
        } usage;

        size_t size;

        const void *initialData = nullptr;
    };

    class RHIBuffer
    {
    public:
        virtual ~RHIBuffer() = default;
        virtual void upload(const void *data, size_t size) = 0;
        virtual size_t getSize() const = 0;
    };
}