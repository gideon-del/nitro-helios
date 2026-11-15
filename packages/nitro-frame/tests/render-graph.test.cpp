#include <catch2/catch_test_macros.hpp>
#include "nitro-frame/graph/render-graph.h"
#include "supports/counting-device.h"
#include <ranges>

using namespace nitro;

TEST_CASE("RAW chain")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto X = renderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    auto &a = renderGraph.addRenderPass("A")
                  .write(X, rhi::ResourceState::ShaderWrite);

    auto &b = renderGraph.addRenderPass("B")
                  .read(X, rhi::ResourceState::ShaderRead);

    REQUIRE_NOTHROW(renderGraph.compile());

    auto *logical = renderGraph.get(X);

    REQUIRE(logical->versions.size() == 1);

    CHECK(logical->versions[0].producer == (int)a.index());

    CHECK(std::ranges::find(logical->versions[0].consumers, (int)b.index()) != logical->versions[0].consumers.end());

    CHECK(renderGraph.orderOf("A") < renderGraph.orderOf("B"));
}
TEST_CASE("WAR chain")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Persistent;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto X = renderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    auto &b = renderGraph.addRenderPass("B")
                  .read(X, rhi::ResourceState::ShaderRead);
    auto &a = renderGraph.addRenderPass("A")
                  .write(X, rhi::ResourceState::ShaderWrite);

    REQUIRE_NOTHROW(renderGraph.compile());

    auto *logical = renderGraph.get(X);

    REQUIRE(logical->versions.size() == 2);

    CHECK(logical->versions[0].producer == frame::kExternalProducer);
    CHECK(logical->versions[1].producer == (int)a.index());

    CHECK(std::ranges::find(logical->versions[0].consumers, (int)b.index()) != logical->versions[0].consumers.end());
    auto orderA = renderGraph.orderOf("A");
    auto orderB = renderGraph.orderOf("B");

    CHECK(orderA > orderB);
}
TEST_CASE("WAW chain")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto X = renderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    auto &a = renderGraph.addRenderPass("A")
                  .write(X, rhi::ResourceState::ShaderWrite);
    auto &b = renderGraph.addRenderPass("B")
                  .write(X, rhi::ResourceState::ShaderWrite);

    auto compiledGraph = renderGraph.compile();

    REQUIRE(compiledGraph.steps.size() == 2);

    auto *logical = renderGraph.get(X);

    REQUIRE(logical->versions.size() == 2);
    CHECK(logical->versions[0].producer == (int)a.index());
    CHECK(logical->versions[1].producer == (int)b.index());

    auto orderA = renderGraph.orderOf("A");
    auto orderB = renderGraph.orderOf("B");

    CHECK(orderA < orderB);
}
TEST_CASE("Same pass writes twice (different mips)")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 2;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto X = renderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    renderGraph.addRenderPass("A")
        .write(X, rhi::ResourceState::ShaderWrite, rhi::TextureSubresource{.baseMip = 0, .mipCount = 1})
        .write(X, rhi::ResourceState::ShaderWrite, rhi::TextureSubresource{.baseMip = 1, .mipCount = 1});

    frame::CompiledRenderGraph compiledGraph;
    REQUIRE_NOTHROW(compiledGraph = renderGraph.compile());

    auto *logical = renderGraph.get(X);
    REQUIRE(compiledGraph.steps.size() == 1);
    REQUIRE(logical->versions.size() == 1);

    CHECK(compiledGraph.steps[0].textureBarriers.size() == 2);
}
TEST_CASE("Read then write in one pass")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 2;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Persistent;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto X = renderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    auto &a = renderGraph.addRenderPass("A")
                  .read(X, rhi::ResourceState::ShaderRead)
                  .write(X, rhi::ResourceState::ShaderWrite);

    REQUIRE_NOTHROW(renderGraph.compile());

    auto *logical = renderGraph.get(X);
    REQUIRE(logical->versions.size() == 2);
    CHECK(logical->versions[0].producer == frame::kExternalProducer);
    CHECK(logical->versions[1].producer == (int)a.index());
    CHECK(std::ranges::find(logical->versions[0].consumers, (int)a.index()) != logical->versions[0].consumers.end());
}

TEST_CASE("Cycle")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 2;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto X = renderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);
    auto Y = renderGraph.createTexture("Y", desc, frame::ResourceSize::textureViewRelative(), options);

    auto &a = renderGraph.addRenderPass("A")
                  .write(X, rhi::ResourceState::ShaderWrite);

    renderGraph.addRenderPass("B")
        .read(X, rhi::ResourceState::ShaderRead)
        .write(Y, rhi::ResourceState::ShaderWrite);
    a.read(Y, rhi::ResourceState::ShaderRead);

    CHECK_THROWS_AS(renderGraph.compile(), std::runtime_error);
}
TEST_CASE("Transient lifetime")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 2;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto X = renderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);
    auto Y = renderGraph.createTexture("Y", desc, frame::ResourceSize::textureViewRelative(), options);
    renderGraph.addRenderPass("A")
        .write(X, rhi::ResourceState::ShaderWrite);
    renderGraph.addRenderPass("B")
        .read(X, rhi::ResourceState::ShaderRead);
    renderGraph.addRenderPass("C")
        .write(Y, rhi::ResourceState::ShaderWrite);

    REQUIRE_NOTHROW(renderGraph.compile());

    auto lifetimes = renderGraph.lifetimes();

    auto *logical = renderGraph.get(X);
    const auto &xLifetime = lifetimes.at(logical->physicalHandle);

    CHECK(xLifetime.createdAt == renderGraph.orderOf("A"));
    CHECK(xLifetime.lastUse == renderGraph.orderOf("B"));
}

TEST_CASE("Temporal previous doesn’t create an edge")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder renderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 2;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Persistent;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;
    options.instanceType = frame::ResourceInstanceType::Temporal;
    options.historyCount = 2;

    auto hiz = renderGraph.createTemporalTexture("Hi-Z", desc, frame::ResourceSize::textureViewRelative(), options);

    auto &a = renderGraph.addRenderPass("A")
                  .write(hiz.current, rhi::ResourceState::ShaderWrite);

    auto &b = renderGraph.addRenderPass("B")
                  .read(hiz.previous, rhi::ResourceState::ShaderRead);

    frame::CompiledRenderGraph compiledGraph;
    REQUIRE_NOTHROW(compiledGraph = renderGraph.compile());

    CHECK(hiz.current != hiz.previous);

    auto *current = renderGraph.get(hiz.current);
    auto *previous = renderGraph.get(hiz.previous);

    REQUIRE(current->versions.size() == 2);
    CHECK(current->versions[1].producer == (int)a.index());
    CHECK(std::ranges::find(previous->versions[0].consumers, (int)b.index()) != previous->versions[0].consumers.end());

    CHECK(compiledGraph.steps.size() == 2);
}

TEST_CASE("Structure Hash")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder firstRenderGraph{resourceManager};
    frame::RenderGraphBuilder secondRenderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 2;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto firstX = firstRenderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);
    auto firstY = firstRenderGraph.createTexture("Y", desc, frame::ResourceSize::textureViewRelative(), options);
    firstRenderGraph.addRenderPass("A")
        .write(firstX, rhi::ResourceState::ShaderWrite);
    firstRenderGraph.addRenderPass("B")
        .read(firstX, rhi::ResourceState::ShaderRead);
    firstRenderGraph.addRenderPass("C")
        .write(firstY, rhi::ResourceState::ShaderWrite);

    auto secondX = secondRenderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);
    auto secondY = secondRenderGraph.createTexture("Y", desc, frame::ResourceSize::textureViewRelative(), options);
    secondRenderGraph.addRenderPass("A")
        .write(secondX, rhi::ResourceState::ShaderWrite);
    secondRenderGraph.addRenderPass("B")
        .read(secondX, rhi::ResourceState::ShaderRead);
    secondRenderGraph.addRenderPass("C")
        .write(secondY, rhi::ResourceState::ShaderWrite);

    CHECK(firstRenderGraph.structureHash() == secondRenderGraph.structureHash());
}

TEST_CASE("RAW vs WAW")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder firstRenderGraph{resourceManager};
    frame::RenderGraphBuilder secondRenderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 2;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto firstX = firstRenderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);
    firstRenderGraph.addRenderPass("A")
        .write(firstX, rhi::ResourceState::ShaderWrite);
    firstRenderGraph.addRenderPass("B")
        .read(firstX, rhi::ResourceState::ShaderWrite);

    auto secondX = secondRenderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    secondRenderGraph.addRenderPass("A")
        .write(secondX, rhi::ResourceState::ShaderWrite);
    secondRenderGraph.addRenderPass("B")
        .write(secondX, rhi::ResourceState::ShaderWrite);

    CHECK(firstRenderGraph.structureHash() != secondRenderGraph.structureHash());
}
TEST_CASE("Texture resource range")
{
    tests::CountingDevice device;

    frame::ResourceManager resourceManager{device};
    frame::RenderGraphBuilder firstRenderGraph{resourceManager};
    frame::RenderGraphBuilder secondRenderGraph{resourceManager};

    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.mipmaps = 3;

    frame::GPUResourceOptions options;
    options.lifetime = frame::ResourceLifetimeType::Transient;
    options.viewHandle = viewHandle;
    options.scope = frame::ResourceScope::View;

    auto firstX = firstRenderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    firstRenderGraph.addRenderPass("A")
        .write(firstX, rhi::ResourceState::ShaderWrite, rhi::TextureSubresource{.baseMip = 0, .mipCount = 1})
        .write(firstX, rhi::ResourceState::ShaderWrite, rhi::TextureSubresource{.baseMip = 1, .mipCount = 1});

    auto secondX = secondRenderGraph.createTexture("X", desc, frame::ResourceSize::textureViewRelative(), options);

    secondRenderGraph.addRenderPass("A")
        .write(secondX, rhi::ResourceState::ShaderWrite, rhi::TextureSubresource{.baseMip = 1, .mipCount = 1})
        .write(secondX, rhi::ResourceState::ShaderWrite, rhi::TextureSubresource{.baseMip = 2, .mipCount = 1});

    CHECK(firstRenderGraph.structureHash() != secondRenderGraph.structureHash());
}