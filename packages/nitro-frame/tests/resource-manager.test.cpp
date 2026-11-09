#include <catch2/catch_test_macros.hpp>
#include "supports/counting-device.h"
#include "nitro-frame/resources/resource-manager.h"

using namespace nitro;

static frame::GPUResourceOptions viewOptions(frame::RenderViewHandle view,
                                             frame::ResourceInstanceType instance = frame::ResourceInstanceType::Single)
{
    frame::GPUResourceOptions o;
    o.scope = frame::ResourceScope::View;
    o.viewHandle = view;
    o.instanceType = instance;
    return o;
}
TEST_CASE("declaring the same resource twice creates it once")
{
    tests::CountingDevice device;
    frame::ResourceManager resources(device);
    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc sceneDesc;
    sceneDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    frame::GPUResourceOptions sceneOptions;
    sceneOptions.scope = frame::ResourceScope::View;
    sceneOptions.viewHandle = viewHandle;
    sceneOptions.instanceType = frame::ResourceInstanceType::Single;

    auto firstHandle = resources.createTexture("SceneColor", sceneDesc, frame::ResourceSize::textureViewRelative(), sceneOptions);
    frame::PhysicalTextureLifetime lifetimes;
    frame::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};

    resources.buildViewResources(buildInfo);

    REQUIRE(device.getCreateCount() == 1);

    auto secondHandle = resources.createTexture("SceneColor", sceneDesc, frame::ResourceSize::textureViewRelative(), sceneOptions);

    resources.buildViewResources(buildInfo);

    CHECK(device.getCreateCount() == 1);

    CHECK(secondHandle == firstHandle);
}
TEST_CASE("resizing rebuilds only view-relative resources, and reports lost history")
{
    tests::CountingDevice device;
    frame::ResourceManager resources(device);
    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    auto aOptions = viewOptions(viewHandle, frame::ResourceInstanceType::Temporal);
    aOptions.historyCount = 2;
    auto bOptions = viewOptions(viewHandle);

    resources.createTexture("A", desc, frame::ResourceSize::textureViewRelative(), aOptions);
    resources.createTexture("B", desc, frame::ResourceSize::textureAbsolute(64, 64), bOptions);

    frame::PhysicalTextureLifetime lifetimes;
    frame::ViewBuildInfo buildInfo{
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
    frame::ResourceManager resources(device);
    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc desc;
    desc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    auto aOptions = viewOptions(viewHandle);
    auto bOptions = viewOptions(viewHandle);

    resources.createTexture("A", desc, frame::ResourceSize::textureViewRelative(), aOptions);
    resources.createTexture("B", desc, frame::ResourceSize::textureViewRelative(), bOptions);

    frame::PhysicalTextureLifetime lifetimes;
    frame::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};

    resources.buildViewResources(buildInfo);

    REQUIRE(device.getCreateCount() == 2);
    resources.createTexture("B", desc, frame::ResourceSize::textureViewRelative(), bOptions);

    desc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
    resources.createTexture("A", desc, frame::ResourceSize::textureViewRelative(), aOptions);

    bool historyReset = resources.buildViewResources(buildInfo);

    CHECK(device.getCreateCount() == 3);
    CHECK(device.getDestroyCount() == 1);
    CHECK(device.getLiveCount() == 2);
    CHECK_FALSE(historyReset);
}
TEST_CASE("destroying a view leaves nothing behind")
{
    tests::CountingDevice device;
    frame::ResourceManager resources(device);
    frame::RenderViewHandle viewHandle{1, 1};

    rhi::TextureDesc textureDesc;
    textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

    auto sceneOptions = viewOptions(viewHandle);
    auto historyOptions = viewOptions(viewHandle, frame::ResourceInstanceType::Temporal);
    historyOptions.historyCount = 2;

    auto cameraUBOOptions = viewOptions(viewHandle, frame::ResourceInstanceType::PerFrame);
    auto scratchOptions = viewOptions(viewHandle, frame::ResourceInstanceType::Single);
    scratchOptions.lifetime = frame::ResourceLifetimeType::Transient;

    rhi::BufferDesc bufferDesc;

    auto oldScene = resources.createTexture("SceneColor", textureDesc, frame::ResourceSize::textureViewRelative(), sceneOptions);
    resources.createTexture("History", textureDesc, frame::ResourceSize::textureViewRelative(), historyOptions);
    auto oldScratch = resources.createTexture("Scratch", textureDesc, frame::ResourceSize::textureViewRelative(), scratchOptions);
    resources.createBuffer("CameraUBO", bufferDesc, frame::ResourceSize::bufferAbsolute(20), cameraUBOOptions);

    frame::PhysicalTextureLifetime lifetimes;
    frame::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};
    resources.buildViewResources(buildInfo);
    REQUIRE(device.getLiveCount() == 5);

    resources.destroyViewResources(viewHandle);
    CHECK(device.getLiveCount() == 0);
    CHECK(device.getDestroyCount() == 5);

    auto newScene = resources.createTexture("SceneColor", textureDesc, frame::ResourceSize::textureViewRelative(), sceneOptions);
    auto newScratch = resources.createTexture("Scratch", textureDesc, frame::ResourceSize::textureViewRelative(), scratchOptions);

    CHECK(newScene != oldScene);
    CHECK(newScratch != oldScratch);
}

TEST_CASE("alias resource")
{

    tests::CountingDevice device;
    frame::ResourceManager resources(device);
    frame::RenderViewHandle viewHandle{1, 1};
    auto opts = viewOptions(viewHandle);
    opts.lifetime = frame::ResourceLifetimeType::Transient;

    rhi::TextureDesc desc;
    auto a = resources.createTexture("A", desc, frame::ResourceSize::textureViewRelative(), opts);
    auto b = resources.createTexture("B", desc, frame::ResourceSize::textureViewRelative(), opts);

    frame::PhysicalTextureLifetime lifetimes;
    lifetimes[a] = {.name = "A", .lastUse = 1, .createdAt = 0};
    lifetimes[b] = {.name = "B", .lastUse = 3, .createdAt = 2};

    frame::ViewBuildInfo buildInfo{
        .handle = viewHandle,
        .extent = {100, 100},
        .structureHash = 0,
        .lifetimes = lifetimes};
    resources.buildViewResources(buildInfo);
    CHECK(device.getHeapCreateCount() == 1);
    CHECK(device.getHeapLiveCount() == 1);
    CHECK(device.getCreateCount() == 2);
    CHECK(device.getLiveCount() == 2);

    resources.buildViewResources(buildInfo);
    CHECK(device.getHeapCreateCount() == 1);
    CHECK(device.getHeapLiveCount() == 1);
    CHECK(device.getCreateCount() == 2);
    CHECK(device.getLiveCount() == 2);

    buildInfo.extent = {200, 200};
    resources.buildViewResources(buildInfo);
    CHECK(device.getHeapCreateCount() == 2);
    CHECK(device.getHeapLiveCount() == 1);
    CHECK(device.getCreateCount() == 4);
    CHECK(device.getLiveCount() == 2);

    buildInfo.extent = {50, 50};
    resources.buildViewResources(buildInfo);
    CHECK(device.getHeapCreateCount() == 2);
    CHECK(device.getHeapLiveCount() == 1);
    CHECK(device.getCreateCount() == 6);
    CHECK(device.getLiveCount() == 2);

    buildInfo.extent = {100, 100};
    resources.buildViewResources(buildInfo);
    CHECK(device.getHeapCreateCount() == 2);
    CHECK(device.getCreateCount() == 8);

    resources.destroyViewResources(viewHandle);
    CHECK(device.getHeapCreateCount() == 2);
    CHECK(device.getHeapLiveCount() == 0);
    CHECK(device.getCreateCount() == 8);
    CHECK(device.getLiveCount() == 0);
}