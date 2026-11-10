#include <catch2/catch_test_macros.hpp>

#include "supports/recording-command-buffer.h"
#include "supports/fake-texture.h"
#include "supports/fake-buffer.h"

using namespace nitro;

TEST_CASE("Uniform barrier")
{
    tests::FakeTexture tex(1, 1);

    tests::RecordingCommandBuffer commandBuffer;

    std::vector<rhi::TextureBarrier> barrier{{.texture = &tex,
                                              .to = rhi::ResourceState::RenderTarget,
                                              .range = rhi::TextureSubresource::all()}};

    commandBuffer.barrier(barrier);

    auto textureBarriers = commandBuffer.resolvedTextureBarriers();
    REQUIRE(textureBarriers.size() == 1);

    auto firstBarrier = textureBarriers[0];

    CHECK(firstBarrier.from == rhi::ResourceState::Undefined);
    CHECK(firstBarrier.to == rhi::ResourceState::RenderTarget);
    CHECK(firstBarrier.range.baseMip == 0);
    CHECK(firstBarrier.range.baseLayer == 0);
    CHECK(firstBarrier.range.layerCount == 1);
    CHECK(firstBarrier.range.mipCount == 1);

    CHECK(tex.state().isUniform());
    CHECK(tex.state().get(0) == rhi::ResourceState::RenderTarget);
}

TEST_CASE("Same read state")
{
    tests::FakeTexture tex(1, 1);

    tests::RecordingCommandBuffer commandBuffer;

    std::vector<rhi::TextureBarrier> barrier{{.texture = &tex,
                                              .to = rhi::ResourceState::ShaderRead,
                                              .range = rhi::TextureSubresource::all()}};

    commandBuffer.barrier(barrier);

    commandBuffer.clearRecording();

    commandBuffer.barrier(barrier);

    CHECK(commandBuffer.barrierEmitCalls() == 0);
}
TEST_CASE("Write → write")
{
    tests::FakeTexture tex(1, 1);

    tests::RecordingCommandBuffer commandBuffer;

    std::vector<rhi::TextureBarrier> barrier{{.texture = &tex,
                                              .to = rhi::ResourceState::ShaderWrite,
                                              .range = rhi::TextureSubresource::all()}};

    commandBuffer.barrier(barrier);

    commandBuffer.clearRecording();

    commandBuffer.barrier(barrier);

    CHECK(commandBuffer.barrierEmitCalls() == 1);

    auto textureBarriers = commandBuffer.resolvedTextureBarriers();
    REQUIRE(textureBarriers.size() == 1);

    auto firstBarrier = textureBarriers[0];

    CHECK(firstBarrier.from == rhi::ResourceState::ShaderWrite);
    CHECK(firstBarrier.to == rhi::ResourceState::ShaderWrite);
    CHECK(commandBuffer.barrierEmitCalls() == 1);
}
TEST_CASE("Mixed split")
{
    tests::FakeTexture tex(4, 1);

    tests::RecordingCommandBuffer commandBuffer;

    std::vector<rhi::TextureBarrier> barrier{
        {
            .texture = &tex,
            .to = rhi::ResourceState::RenderTarget,
            .range = rhi::TextureSubresource{
                .baseMip = 0,
                .mipCount = 2,
                .baseLayer = 0,
                .layerCount = 1},

        },
        {
            .texture = &tex,
            .to = rhi::ResourceState::ShaderRead,
            .range = rhi::TextureSubresource{.baseMip = 2, .mipCount = 2, .baseLayer = 0, .layerCount = 1},

        }

    };

    commandBuffer.barrier(barrier);
    REQUIRE_FALSE(tex.state().isUniform());
    commandBuffer.clearRecording();
    barrier = {{
        .texture = &tex,
        .to = rhi::ResourceState::CopySrc,
        .range = rhi::TextureSubresource::all(),

    }};

    commandBuffer.barrier(barrier);

    auto textureBarriers = commandBuffer.resolvedTextureBarriers();
    REQUIRE(textureBarriers.size() == 2);

    auto firstBarrier = textureBarriers[0];
    auto secondBarrier = textureBarriers[1];

    CHECK(firstBarrier.from == rhi::ResourceState::RenderTarget);
    CHECK(firstBarrier.to == rhi::ResourceState::CopySrc);
    CHECK(secondBarrier.from == rhi::ResourceState::ShaderRead);
    CHECK(secondBarrier.to == rhi::ResourceState::CopySrc);
    CHECK(firstBarrier.range.baseMip == 0);
    CHECK(firstBarrier.range.mipCount == 2);
    CHECK(secondBarrier.range.baseMip == 2);
    CHECK(secondBarrier.range.mipCount == 2);

    SECTION("Collapses back to uniform")
    {
        CHECK(tex.state().isUniform());
        CHECK(tex.state().get(0) == rhi::ResourceState::CopySrc);
    }
}
TEST_CASE("Uniform cube, one mip")
{
    tests::FakeTexture tex(3, 6);

    tests::RecordingCommandBuffer commandBuffer;

    std::vector<rhi::TextureBarrier> barrier{
        {
            .texture = &tex,
            .to = rhi::ResourceState::ShaderRead,
            .range = rhi::TextureSubresource{
                .baseMip = 1,
                .mipCount = 1,
            },

        }};

    commandBuffer.barrier(barrier);
    auto textureBarriers = commandBuffer.resolvedTextureBarriers();
    REQUIRE(textureBarriers.size() == 1);
    auto firstBarrier = textureBarriers[0];

    CHECK(firstBarrier.range.baseMip == 1);
    CHECK(firstBarrier.range.mipCount == 1);
    CHECK(firstBarrier.range.baseLayer == 0);
    CHECK(firstBarrier.range.layerCount == 6);
    CHECK(!tex.state().isUniform());
}

TEST_CASE("Discard")
{
    tests::FakeTexture tex(3, 6);

    tests::RecordingCommandBuffer commandBuffer;

    std::vector<rhi::TextureBarrier> barrier{
        {
            .texture = &tex,
            .to = rhi::ResourceState::ShaderRead,
            .range = rhi::TextureSubresource::all(),

        }};

    commandBuffer.barrier(barrier);

    commandBuffer.discard(tex);

    barrier[0].to = rhi::ResourceState::RenderTarget;

    commandBuffer.clearRecording();
    commandBuffer.barrier(barrier);
    auto textureBarriers = commandBuffer.resolvedTextureBarriers();
    REQUIRE(textureBarriers.size() == 1);
    auto firstBarrier = textureBarriers[0];

    CHECK(firstBarrier.from == rhi::ResourceState::Undefined);
}
TEST_CASE("Buffer barrier")
{
    tests::FakeBuffer buffer;

    tests::RecordingCommandBuffer commandBuffer;

    std::vector<rhi::BufferBarrier> barrier{
        {
            .buffer = &buffer,
            .to = rhi::ResourceState::ShaderWrite,

        }};

    commandBuffer.barrier({}, barrier);

    commandBuffer.clearRecording();

    barrier[0].to = rhi::ResourceState::ShaderRead;

    commandBuffer.barrier({}, barrier);

    auto bufferBarrier = commandBuffer.resolvedBufferBarriers();

    REQUIRE(bufferBarrier.size() == 1);

    CHECK(bufferBarrier[0].from == rhi::ResourceState::ShaderWrite);

    commandBuffer.clearRecording();
    commandBuffer.barrier({}, barrier);

    CHECK(commandBuffer.barrierEmitCalls() == 0);
}