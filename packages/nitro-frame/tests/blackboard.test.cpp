#include <catch2/catch_test_macros.hpp>
#include "nitro-frame/graph/blackboard.h"

using namespace nitro;

namespace
{
    struct A
    {
        int value = 0;
    };
    struct B
    {
        float value = 0.0f;
    };
    struct Missing
    {
    };
    struct WithCtor
    {
        int x;
        int y;
    };
} // namespace

TEST_CASE("Blackboard: references survive later adds")
{
    frame::Blackboard bb;

    auto &a = bb.add<A>();
    a.value = 42;

    // Several more inserts can trigger a rehash; node-based storage must keep `a` valid.
    bb.add<B>();
    bb.add<WithCtor>(1, 2);

    CHECK(a.value == 42);
    CHECK(&a == &bb.get<A>());
}

TEST_CASE("Blackboard: tryGet returns nullptr for a missing type")
{
    frame::Blackboard bb;
    bb.add<A>();

    CHECK(bb.tryGet<Missing>() == nullptr);
    CHECK(bb.tryGet<A>() != nullptr);
}

TEST_CASE("Blackboard: cv-ref qualified types resolve to the same entry")
{
    frame::Blackboard bb;
    auto &a = bb.add<A>();

    CHECK(&bb.get<const A>() == &a);
    CHECK(&bb.get<A &>() == &a);
    CHECK(bb.tryGet<const A &>() == &a);

    const frame::Blackboard &cbb = bb;
    CHECK(&cbb.get<A>() == &a);
    CHECK(cbb.tryGet<A>() == &a);
}

TEST_CASE("Blackboard: add forwards constructor arguments")
{
    frame::Blackboard bb;
    auto &w = bb.add<WithCtor>(3, 4); // C++20 parenthesised aggregate init

    CHECK(w.x == 3);
    CHECK(w.y == 4);
}

TEST_CASE("Blackboard: clear removes every entry")
{
    frame::Blackboard bb;
    bb.add<A>();
    bb.add<B>();

    bb.clear();

    CHECK(bb.tryGet<A>() == nullptr);
    CHECK(bb.tryGet<B>() == nullptr);

    // A type can be added again after clear.
    bb.add<A>().value = 7;
    CHECK(bb.get<A>().value == 7);
}