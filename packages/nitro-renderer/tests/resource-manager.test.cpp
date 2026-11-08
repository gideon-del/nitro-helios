#include <catch2/catch_test_macros.hpp>
#include "supports/counting-device.h"
#include "nitro-renderer/graph/resource-manager.h"

using namespace nitro;

static renderer::GPUResourceOptions viewOptions(renderer::RenderViewHandle view,
                                                renderer::ResourceInstanceType instance = renderer::ResourceInstanceType::Single)
{
    renderer::GPUResourceOptions o;
    o.scope = renderer::ResourceScope::View;
    o.viewHandle = view;
    o.instanceType = instance;
    return o;
}
TEST_CASE("declaring the same resource twice creates it once")
{
    tests::CountingDevice device;
    renderer::ResourceManager resources(device);
    renderer::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc sceneDesc;
    sceneDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    renderer::GPUResourceOptions sceneOptions;
    sceneOptions.scope = renderer::ResourceScope::View;
    sceneOptions.viewHandle = viewHandle;
    sceneOptions.instanceType = renderer::ResourceInstanceType::Single;

    auto firstHandle = resources.createTexture("SceneColor", sceneDesc, renderer::ResourceSize::textureViewRelative(), sceneOptions);
    renderer::PhysicalTextureLifetime lifetimes;
    renderer::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};

    resources.buildViewResources(buildInfo);

    REQUIRE(device.getCreateCount() == 1);

    auto secondHandle = resources.createTexture("SceneColor", sceneDesc, renderer::ResourceSize::textureViewRelative(), sceneOptions);

    resources.buildViewResources(buildInfo);

    CHECK(device.getCreateCount() == 1);

    CHECK(secondHandle == firstHandle);
}
TEST_CASE("resizing rebuilds only view-relative resources, and reports lost history")
{
    tests::CountingDevice device;
    renderer::ResourceManager resources(device);
    renderer::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    auto aOptions = viewOptions(viewHandle, renderer::ResourceInstanceType::Temporal);
    aOptions.historyCount = 2;
    auto bOptions = viewOptions(viewHandle);

    resources.createTexture("A", desc, renderer::ResourceSize::textureViewRelative(), aOptions);
    resources.createTexture("B", desc, renderer::ResourceSize::textureAbsolute(64, 64), bOptions);

    renderer::PhysicalTextureLifetime lifetimes;
    renderer::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};

    resources.buildViewResources(buildInfo);

    REQUIRE(device.getCreateCount() == 3);

    buildInfo.extent = {200, 200};

    bool historyReset = resources.buildViewResources(buildInfo);

    CHECK(historyReset);

    CHECK(device.getCreateCount() == 5);
    CHECK(device.getDestroyCount() == 2);
    CHECK(device.getLiveCount() == 3);

    historyReset = resources.buildViewResources(buildInfo);
    CHECK_FALSE(historyReset);
    CHECK(device.getCreateCount() == 5);
}

TEST_CASE("If you change a resource’s description, that resource, and only that one, is rebuilt")
{
    tests::CountingDevice device;
    renderer::ResourceManager resources(device);
    renderer::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    auto aOptions = viewOptions(viewHandle);
    auto bOptions = viewOptions(viewHandle);

    resources.createTexture("A", desc, renderer::ResourceSize::textureViewRelative(), aOptions);
    resources.createTexture("B", desc, renderer::ResourceSize::textureViewRelative(), bOptions);

    renderer::PhysicalTextureLifetime lifetimes;
    renderer::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};

    resources.buildViewResources(buildInfo);

    REQUIRE(device.getCreateCount() == 2);
    resources.createTexture("B", desc, renderer::ResourceSize::textureViewRelative(), bOptions);

    desc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
    resources.createTexture("A", desc, renderer::ResourceSize::textureViewRelative(), aOptions);

    bool historyReset = resources.buildViewResources(buildInfo);

    CHECK(device.getCreateCount() == 3);
    CHECK(device.getDestroyCount() == 1);
    CHECK(device.getLiveCount() == 2);
    CHECK_FALSE(historyReset);
}
TEST_CASE("destroying a view leaves nothing behind")
{
    tests::CountingDevice device;
    renderer::ResourceManager resources(device);
    renderer::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc textureDesc;
    textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    auto sceneOptions = viewOptions(viewHandle);
    auto historyOptions = viewOptions(viewHandle, renderer::ResourceInstanceType::Temporal);
    historyOptions.historyCount = 2;

    auto cameraUBOOptions = viewOptions(viewHandle, renderer::ResourceInstanceType::PerFrame);
    auto scratchOptions = viewOptions(viewHandle, renderer::ResourceInstanceType::Single);
    scratchOptions.lifetime = renderer::ResourceLifetimeType::Transient;

    rhi::BufferDesc bufferDesc;

    auto oldScene = resources.createTexture("SceneColor", textureDesc, renderer::ResourceSize::textureViewRelative(), sceneOptions);
    resources.createTexture("History", textureDesc, renderer::ResourceSize::textureViewRelative(), historyOptions);
    auto oldScratch = resources.createTexture("Scratch", textureDesc, renderer::ResourceSize::textureViewRelative(), scratchOptions);
    resources.createBuffer("CameraUBO", bufferDesc, renderer::ResourceSize::bufferAbsolute(20), cameraUBOOptions);

    renderer::PhysicalTextureLifetime lifetimes;
    renderer::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};
    resources.buildViewResources(buildInfo);
    REQUIRE(device.getLiveCount() == 5);

    resources.destroyViewResources(viewHandle);
    CHECK(device.getLiveCount() == 0);
    CHECK(device.getDestroyCount() == 5);

    auto newScene = resources.createTexture("SceneColor", textureDesc, renderer::ResourceSize::textureViewRelative(), sceneOptions);
    auto newScratch = resources.createTexture("Scratch", textureDesc, renderer::ResourceSize::textureViewRelative(), scratchOptions);

    CHECK(newScene != oldScene);
    CHECK(newScratch != oldScratch);
}