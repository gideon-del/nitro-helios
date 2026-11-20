#include <catch2/catch_test_macros.hpp>
#include "fake-texture.h"
#include "recording-command-buffer.h"
#include "nitro-frame/view/view-system.h"
#include "nitro-frame/graph/render-graph.h"
#include "counting-device.h"
#include "nitro-rhi/rhi-constants.h"

using namespace nitro;

namespace
{
    const frame::RenderPassExecute noop = [](const frame::PassContext &) {};

    // Declares one persistent, view-relative output written by a single pass.
    // Records what the view looked like at declare time.
    struct SimpleGraph
    {
        bool sawHistoryValid = false;

        frame::ViewBuildFn fn()
        {
            return [this](frame::RenderGraphBuilder &b, const frame::RenderView &v)
            {
                sawHistoryValid = v.historyValid;

                frame::GPUResourceOptions options;
                options.lifetime = frame::ResourceLifetimeType::Persistent;
                options.scope = frame::ResourceScope::View;
                options.viewHandle = v.handle;

                auto out = b.createTexture("Out", rhi::TextureDesc{}, frame::ResourceSize::textureViewRelative(), options);
                b.addRenderPass("Write", noop).write(out, rhi::ResourceState::ShaderWrite);
                return out;
            };
        }
    };

    frame::FrameInfo makeFrame(rhi::RHICommandBuffer &cmd, uint64_t frameNumber)
    {
        return frame::FrameInfo{
            .cmd = cmd,
            .frameIndex = static_cast<uint32_t>(frameNumber % rhi::kMaxFramesInFlight),
            .frameNumber = frameNumber};
    }
} // namespace

TEST_CASE("View: history invalid on first frame, valid on the next")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;
    SimpleGraph graph;

    auto view = views.createView({100, 100});

    views.render(view, makeFrame(cmd, 1), graph.fn());
    CHECK_FALSE(graph.sawHistoryValid);

    views.render(view, makeFrame(cmd, 2), graph.fn());
    CHECK(graph.sawHistoryValid);
}

TEST_CASE("View: skipped frame invalidates history")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;
    SimpleGraph graph;

    auto view = views.createView({100, 100});

    views.render(view, makeFrame(cmd, 1), graph.fn());
    views.render(view, makeFrame(cmd, 2), graph.fn());
    REQUIRE(graph.sawHistoryValid);

    views.render(view, makeFrame(cmd, 4), graph.fn());
    CHECK_FALSE(graph.sawHistoryValid);

    views.render(view, makeFrame(cmd, 5), graph.fn());
    CHECK(graph.sawHistoryValid);
}

TEST_CASE("View: resize invalidates history for one frame and rebuilds the output")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;
    SimpleGraph graph;

    auto view = views.createView({100, 100});

    views.render(view, makeFrame(cmd, 1), graph.fn());
    views.render(view, makeFrame(cmd, 2), graph.fn());
    REQUIRE(graph.sawHistoryValid);
    REQUIRE(device.getCreateCount() == 1);

    views.resize(view, {200, 100});
    CHECK_FALSE(views.getView(view)->historyValid);

    views.render(view, makeFrame(cmd, 3), graph.fn());
    CHECK_FALSE(graph.sawHistoryValid);
    CHECK(device.getCreateCount() == 2); // view-relative output rebuilt at new extent

    views.render(view, makeFrame(cmd, 4), graph.fn());
    CHECK(graph.sawHistoryValid);
}

TEST_CASE("View: resize to the same extent is a no-op")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;
    SimpleGraph graph;

    auto view = views.createView({100, 100});

    views.render(view, makeFrame(cmd, 1), graph.fn());
    views.resize(view, {100, 100});
    views.render(view, makeFrame(cmd, 2), graph.fn());

    CHECK(graph.sawHistoryValid);
    CHECK(device.getCreateCount() == 1);
}

TEST_CASE("View: same graph across frames creates no new resources")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;
    SimpleGraph graph;

    auto view = views.createView({100, 100});

    auto first = views.render(view, makeFrame(cmd, 1), graph.fn());
    auto second = views.render(view, makeFrame(cmd, 2), graph.fn());
    auto third = views.render(view, makeFrame(cmd, 3), graph.fn());

    CHECK(device.getCreateCount() == 1);
    CHECK(first == second);
    CHECK(second == third);
}

TEST_CASE("View: temporal output rotates through render")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;

    auto build = [](frame::RenderGraphBuilder &b, const frame::RenderView &v)
    {
        frame::GPUResourceOptions options;
        options.lifetime = frame::ResourceLifetimeType::Persistent;
        options.scope = frame::ResourceScope::View;
        options.viewHandle = v.handle;
        options.instanceType = frame::ResourceInstanceType::Temporal;
        options.historyCount = 2;

        auto h = b.createTemporalTexture("H", rhi::TextureDesc{}, frame::ResourceSize::textureViewRelative(), options);
        b.addRenderPass("Write", noop).write(h.current, rhi::ResourceState::ShaderWrite);
        return h.current;
    };

    auto view = views.createView({100, 100});

    auto f1 = views.render(view, makeFrame(cmd, 1), build);
    auto f2 = views.render(view, makeFrame(cmd, 2), build);
    auto f3 = views.render(view, makeFrame(cmd, 3), build);

    CHECK(f1 != f2); // render advanced frameIdx
    CHECK(f1 == f3); // and it wraps at historyCount
    CHECK(device.getCreateCount() == 2);
}

TEST_CASE("View: final barrier transitions the output to ShaderRead")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;
    SimpleGraph graph;

    auto view = views.createView({100, 100});
    views.render(view, makeFrame(cmd, 1), graph.fn());

    auto &barriers = cmd.resolvedTextureBarriers();
    REQUIRE_FALSE(barriers.empty());
    CHECK(barriers.back().to == rhi::ResourceState::ShaderRead);
}

TEST_CASE("View: destroy frees resources and invalidates the handle")
{
    tests::CountingDevice device;
    frame::ResourceManager rm{device};
    frame::ViewSystem views{rm};
    tests::RecordingCommandBuffer cmd;
    SimpleGraph graph;

    auto view = views.createView({100, 100});
    views.render(view, makeFrame(cmd, 1), graph.fn());

    views.destroyView(view);

    CHECK(views.getView(view) == nullptr);
    CHECK(device.getDestroyCount() == 1);

    REQUIRE_NOTHROW(views.destroyView(view)); // stale handle is ignored
}