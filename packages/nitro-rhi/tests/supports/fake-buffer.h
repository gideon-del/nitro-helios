#pragma once
#include "nitro-rhi/rhi-buffer.h"

namespace nitro::tests
{
    using namespace rhi;

    class FakeBuffer : public RHIBuffer
    {
    public:
        void upload(const void *data, size_t size, size_t offset = 0) override {}
        size_t getSize() const override { return 1; }
        void *map() override { return nullptr; }
        void unmap() override {};
    };
}