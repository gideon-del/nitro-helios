#include <catch2/catch_test_macros.hpp>
#include "supports/counting-device.h"
#include "nitro-rhi/pipeline-cache.h"

using namespace nitro;
TEST_CASE("Cache should return same pipeline")
{
    tests::CountingDevice device;

    SECTION("Shaders in a different order")
    {
        rhi::PipelineCache cache{device};
        rhi::PipelineDesc firstDesc;
        firstDesc.shaders = {
            {.name = "fragment",
             .filePath = "/test",
             .stage = rhi::ShaderStage::Fragment},
            {.name = "vertex",
             .filePath = "/test",
             .stage = rhi::ShaderStage::Vertex}};
        auto a = cache.get(firstDesc);

        rhi::PipelineDesc secondDesc;
        secondDesc.shaders = {
            {.name = "vertex",
             .filePath = "/test",
             .stage = rhi::ShaderStage::Vertex},
            {.name = "fragment",
             .filePath = "/test",
             .stage = rhi::ShaderStage::Fragment}};

        auto b = cache.get(secondDesc);

        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);

        CHECK(a == b);
    }
    SECTION("Vertex attributes in a different order")
    {
        rhi::PipelineCache cache{device};
        rhi::PipelineDesc firstDesc;
        firstDesc.vertexLayout.attributes = {
            {.location = 0,
             .offset = 0,
             .format = rhi::RHIVertexLayout::Attributes::Format::Float},
            {.location = 1,
             .offset = 4,
             .format = rhi::RHIVertexLayout::Attributes::Format::Float},
        };
        auto a = cache.get(firstDesc);

        rhi::PipelineDesc secondDesc;
        secondDesc.vertexLayout.attributes = {
            {.location = 1,
             .offset = 4,
             .format = rhi::RHIVertexLayout::Attributes::Format::Float},
            {.location = 0,
             .offset = 0,
             .format = rhi::RHIVertexLayout::Attributes::Format::Float},

        };

        auto b = cache.get(secondDesc);

        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);

        CHECK(a == b);
    }
    SECTION("Only Stencil Reference differ")
    {
        rhi::PipelineCache cache{device};
        rhi::PipelineDesc firstDesc;
        firstDesc.stencil.front.reference = 10;
        firstDesc.stencil.back.reference = 10;
        auto a = cache.get(firstDesc);

        rhi::PipelineDesc secondDesc;
        secondDesc.stencil.front.reference = 40;
        secondDesc.stencil.back.reference = 40;

        auto b = cache.get(secondDesc);

        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);

        CHECK(a == b);
    }
}
TEST_CASE("Cache should return different pipeline")
{
    tests::CountingDevice device;

    SECTION("Colour attachments swapped")
    {
        rhi::PipelineCache cache{device};
        rhi::PipelineDesc firstDesc;
        firstDesc.colorAttachments = {
            {rhi::TextureDesc::ImageFormat::ColorRGBA8},
            {rhi::TextureDesc::ImageFormat::ColorRGBA16},
        };
        auto a = cache.get(firstDesc);

        rhi::PipelineDesc secondDesc;
        secondDesc.colorAttachments = {
            {rhi::TextureDesc::ImageFormat::ColorRGBA16},
            {rhi::TextureDesc::ImageFormat::ColorRGBA8},
        };

        auto b = cache.get(secondDesc);

        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);

        CHECK(a != b);
    }
    SECTION("Layouts swapped")
    {
        tests::FakeDescriptorLayout firstLayout;
        tests::FakeDescriptorLayout secondLayout;
        rhi::PipelineCache cache{device};
        rhi::PipelineDesc firstDesc;
        firstDesc.layouts = {
            &firstLayout,
            &secondLayout};
        auto a = cache.get(firstDesc);

        rhi::PipelineDesc secondDesc;
        secondDesc.layouts = {
            &secondLayout,
            &firstLayout,
        };

        auto b = cache.get(secondDesc);

        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);

        CHECK(a != b);
    }
}

namespace
{
    rhi::PipelineDesc basicDesc()
    {
        rhi::PipelineDesc desc;
        desc.shaders = {
            {.name = "vertex", .filePath = "/test", .stage = rhi::ShaderStage::Vertex},
            {.name = "fragment", .filePath = "/test", .stage = rhi::ShaderStage::Fragment}};
        desc.colorAttachments = {{rhi::TextureDesc::ImageFormat::ColorRGBA8}};
        return desc;
    }

    rhi::ComputePipelineDesc basicComputeDesc()
    {
        rhi::ComputePipelineDesc desc;
        desc.shader = {.name = "main", .filePath = "/compute", .stage = rhi::ShaderStage::Compute};
        return desc;
    }
} // namespace

TEST_CASE("Cache creates a pipeline only once per unique desc")
{
    tests::CountingDevice device;
    rhi::PipelineCache cache{device};

    auto a = cache.get(basicDesc());
    auto b = cache.get(basicDesc());

    REQUIRE(a != nullptr);
    CHECK(a == b);
    CHECK(device.pipelineCreateCount() == 1);
}

TEST_CASE("A single differing field creates a new pipeline")
{
    tests::CountingDevice device;
    rhi::PipelineCache cache{device};

    auto base = basicDesc();
    auto other = basicDesc();
    other.cullMode = rhi::PipelineDesc::CullMode::None;

    CHECK(cache.get(base) != cache.get(other));
    CHECK(device.pipelineCreateCount() == 2);
}

TEST_CASE("Cache destroys every pipeline exactly once")
{
    tests::CountingDevice device;
    {
        rhi::PipelineCache cache{device};

        auto other = basicDesc();
        other.cullMode = rhi::PipelineDesc::CullMode::None;

        cache.get(basicDesc());
        cache.get(basicDesc()); // hit
        cache.get(other);

        auto otherCompute = basicComputeDesc();
        otherCompute.threadGroupSizeX = 8;

        cache.get(basicComputeDesc());
        cache.get(basicComputeDesc()); // hit
        cache.get(otherCompute);

        CHECK(device.pipelineDestroyCount() == 0);
        CHECK(device.computePipelineDestroyCount() == 0);
    }
    CHECK(device.pipelineCreateCount() == 2);
    CHECK(device.pipelineDestroyCount() == 2);
    CHECK(device.computePipelineCreateCount() == 2);
    CHECK(device.computePipelineDestroyCount() == 2);
}

TEST_CASE("Device receives the normalised desc")
{
    tests::CountingDevice device;
    rhi::PipelineCache cache{device};

    rhi::PipelineDesc desc = basicDesc();

    auto *pipeline = static_cast<tests::FakePipeline *>(cache.get(desc));
    REQUIRE(pipeline != nullptr);
    REQUIRE(pipeline->desc.shaders.size() == 2);

    rhi::PipelineKey expected{desc};
    CHECK(pipeline->desc.shaders == expected.desc.shaders);
    CHECK(pipeline->desc.shaders != desc.shaders);
}

TEST_CASE("Compute cache: same desc hits, different desc misses")
{
    tests::CountingDevice device;
    rhi::PipelineCache cache{device};

    auto a = cache.get(basicComputeDesc());
    auto b = cache.get(basicComputeDesc());

    auto other = basicComputeDesc();
    other.threadGroupSizeX = 8;
    auto c = cache.get(other);

    REQUIRE(a != nullptr);
    CHECK(a == b);
    CHECK(a != c);
    CHECK(device.computePipelineCreateCount() == 2);
}