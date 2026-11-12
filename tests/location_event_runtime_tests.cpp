#include "minidayz/location_event_runtime.hpp"
#include "minidayz/world.hpp"
#include "minidayz/generated/location_template_source.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <random>

using namespace minidayz;

namespace {

void test_all_original_location_branches() {
    static_assert(stage1::kLocationTemplateSourceCount == 62);
    construct::Array map_array;
    assert(map_array.set_size(16, 16, 1));

    std::size_t executions = 0;
    for (const auto& location : stage1::kLocationTemplateSource) {
        assert(location.tile_code_count > 0);
        for (std::uint8_t level = 0; level <= 5; ++level) {
            for (std::size_t code_index = 0;
                 code_index < location.tile_code_count; ++code_index) {
                std::mt19937 branch_rng(
                    0x4d445a33u + static_cast<std::uint32_t>(executions * 7919u));
                construct::SourceLocationEventRuntime branch_runtime;
                assert(branch_runtime.initialize(level, map_array, branch_rng));
                branch_runtime.set_mapgen(location.tile_codes[code_index],
                    static_cast<std::uint8_t>(code_index % 16),
                    static_cast<std::uint8_t>((code_index / 16) % 16),
                    1000.0 + code_index * 100.0,
                    2000.0 + code_index * 100.0, 100.0, 100.0);
                bool matched = false;
                if (!branch_runtime.execute_branch(location.event_uid, matched)) {
                    std::fprintf(stderr,
                        "Original location branch failed: uid=%llu code=%u level=%u\\n",
                        static_cast<unsigned long long>(location.event_uid),
                        static_cast<unsigned>(location.tile_codes[code_index]),
                        static_cast<unsigned>(level));
                    assert(false);
                }
                ++executions;
            }
        }
    }
    // 62 authored branches, all tile-code variants, across all six source levels.
    assert(executions >= stage1::kLocationTemplateSourceCount * 6);
}

} // namespace

int main() {
    test_all_original_location_branches();

    construct::Array map_array;
    assert(map_array.set_size(16, 16, 1));

    std::mt19937 rng(12345);
    construct::SourceLocationEventRuntime runtime;
    assert(runtime.initialize(0, map_array, rng));
    runtime.set_mapgen(50, 3, 4, 1000.0, 2000.0, 100.0, 100.0);

    const auto* location = find_source_location_template(50);
    assert(location);
    bool matched = false;
    assert(runtime.execute_branch(location->event_uid, matched));
    assert(matched);

    assert(runtime.location_array().height() == 6);
    const auto& spawns = runtime.spawns();
    const auto zed = std::find_if(spawns.begin(), spawns.end(),
        [](const construct::SourceLocationSpawn& spawn) {
            return spawn.object == 216 && spawn.layer == "zed_resp";
        });
    assert(zed != spawns.end());
    assert(std::fabs(zed->x - 1237.0) < 0.001);
    assert(std::fabs(zed->y - 2283.0) < 0.001);
    assert(zed->frame == 11);

    // This branch exercises choose(), random(), floor(), int(), arithmetic,
    // Array writes, and the source Repeat(16) loop.
    runtime.set_mapgen(52, 4, 5, 1500.0, 2500.0, 100.0, 100.0);
    const auto* repeated_location = find_source_location_template(52);
    assert(repeated_location);
    assert(runtime.execute_branch(repeated_location->event_uid, matched));
    assert(matched);
    assert(runtime.location_array().height() == 7);
    assert(runtime.spawns().size() == 1);
    return 0;
}
