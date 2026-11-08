#include <catch2/catch_test_macros.hpp>
#include "nitro-renderer/graph/resource-types.h"
#include "nitro-renderer/graph/alias-planner.h"
#include <random>
#include <algorithm>
using namespace nitro;
constexpr size_t MiB = 1024 * 1024;
constexpr size_t KiB = 1024;

static renderer::TransientEntry entry(uint32_t index, int first, int last,
                                      size_t size, size_t alignment = 64 * KiB, uint32_t bits = 0b011)
{
    return {.handle = {index, 1}, .first = first, .last = last, .requirements = {.size = size, .alignment = alignment, .memoryTypeBits = bits}};
}

static size_t totalBytes(const renderer::AliasPlan &plan)
{
    size_t total = 0;
    for (auto &[id, group] : plan.groups)
        total += group.requirement.size;
    return total;
}

TEST_CASE("the frame table", "[alias]")
{
    std::vector<renderer::TransientEntry> entries = {
        entry(1, 0, 2, 16 * MiB),
        entry(2, 0, 2, 16 * MiB),
        entry(3, 3, 4, 4 * MiB),
        entry(4, 5, 6, 8 * MiB),
        entry(5, 3, 7, 2 * MiB, 4 * KiB, 0b100),
    };

    auto plan = renderer::planAliases(entries);

    SECTION("packs into three heaps")
    {

        CHECK(plan.groups.size() == 3);
        CHECK(totalBytes(plan) == 34 * MiB);
        CHECK(plan.textureToHeap.at(entries[0].handle) != plan.textureToHeap.at(entries[1].handle));

        auto readbackHeap = plan.textureToHeap.at(entries[4].handle);
        CHECK(plan.groups.at(readbackHeap).textures.size() == 1);
    }

    SECTION("shuffling gives the same plan")
    {
        auto shuffled = entries;
        std::mt19937 rng(42);
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        CHECK(renderer::planAliases(shuffled).textureToHeap == plan.textureToHeap);
    }
}
TEST_CASE("memory type bits narrow as members join a group", "[alias]")
{
    std::vector<renderer::TransientEntry> entries = {
        entry(1, 0, 1, 4 * MiB, 4 * KiB, 0b011),
        entry(2, 2, 3, 4 * MiB, 4 * KiB, 0b110),
        entry(3, 4, 5, 4 * MiB, 4 * KiB, 0b001),

    };

    auto plan = renderer::planAliases(entries);

    CHECK(plan.groups.size() == 2);
    CHECK(plan.textureToHeap.at(entries[0].handle) == plan.textureToHeap.at(entries[1].handle));

    auto abGroup = plan.groups[plan.textureToHeap.at(entries[0].handle)];

    CHECK(abGroup.requirement.memoryTypeBits == 0b010);
}
TEST_CASE("ties go to the smaller heap, keeping big heaps free", "[alias]")
{
    std::vector<renderer::TransientEntry> entries = {
        entry(1, 0, 1, 16 * MiB),
        entry(2, 0, 1, 4 * MiB),
        entry(3, 2, 3, 2 * MiB),
        entry(4, 2, 3, 12 * MiB),

    };

    auto plan = renderer::planAliases(entries);

    CHECK(totalBytes(plan) == 20 * MiB);

    CHECK(plan.textureToHeap.at(entries[1].handle) == plan.textureToHeap.at(entries[2].handle));
    CHECK(plan.textureToHeap.at(entries[0].handle) == plan.textureToHeap.at(entries[3].handle));
    CHECK(plan.groups.size() == 2);
}
