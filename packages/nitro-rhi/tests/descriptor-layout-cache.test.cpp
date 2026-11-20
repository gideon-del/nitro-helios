#include <catch2/catch_test_macros.hpp>
#include "nitro-rhi/descriptor-layout-cache.h"
#include "supports/counting-device.h"
#include <vector>

using namespace nitro;
using Binding = rhi::RHIDescriptorBinding;

namespace
{

    Binding sampler(uint32_t slot, Binding::ShaderStage stage = Binding::ShaderStage::Fragment)
    {
        Binding b{};
        b.type = Binding::Type::Sampler;
        b.stage = stage;
        b.binding = slot;
        return b;
    }
} // namespace

TEST_CASE("LayoutCache: same bindings return the same layout")
{
    tests::CountingDevice device;
    rhi::DescriptorLayoutCache cache{device};

    std::vector<Binding> bindings{sampler(0), sampler(1)};

    auto *a = cache.get(bindings);
    auto *b = cache.get(bindings);

    REQUIRE(a != nullptr);
    CHECK(a == b);
    CHECK(device.layoutCreateCount() == 1);
}

TEST_CASE("LayoutCache: declaration order does not matter")
{
    tests::CountingDevice device;
    rhi::DescriptorLayoutCache cache{device};

    std::vector<Binding> ordered{sampler(0), sampler(1), sampler(2)};
    std::vector<Binding> shuffled{sampler(2), sampler(0), sampler(1)};

    CHECK(cache.get(ordered) == cache.get(shuffled));
    CHECK(device.layoutCreateCount() == 1);
}

TEST_CASE("LayoutCache: any differing field creates a new layout")
{
    tests::CountingDevice device;
    rhi::DescriptorLayoutCache cache{device};

    std::vector<Binding> base{sampler(0), sampler(1)};
    auto *baseLayout = cache.get(base);

    SECTION("different binding slot")
    {
        std::vector<Binding> other{sampler(0), sampler(2)};
        CHECK(cache.get(other) != baseLayout);
    }

    SECTION("different stage")
    {
        std::vector<Binding> other{sampler(0), sampler(1, Binding::ShaderStage::Vertex)};
        CHECK(cache.get(other) != baseLayout);
    }

    SECTION("different count")
    {
        std::vector<Binding> other{sampler(0)};
        CHECK(cache.get(other) != baseLayout);
    }

    SECTION("bindless flag")
    {
        std::vector<Binding> other = base;
        other[1].isBindlessArray = true;
        other[1].bindlessCount = 128;
        CHECK(cache.get(other) != baseLayout);
    }

    CHECK(device.layoutCreateCount() == 2);
}

TEST_CASE("LayoutCache: destroys every layout exactly once")
{
    tests::CountingDevice device;
    {
        rhi::DescriptorLayoutCache cache{device};
        cache.get(std::vector<Binding>{sampler(0)});
        cache.get(std::vector<Binding>{sampler(0)}); // hit, not a new layout
        cache.get(std::vector<Binding>{sampler(0), sampler(1)});
        CHECK(device.layoutDestroyCount() == 0);
    }
    CHECK(device.layoutCreateCount() == 2);
    CHECK(device.layoutDestroyCount() == 2);
}