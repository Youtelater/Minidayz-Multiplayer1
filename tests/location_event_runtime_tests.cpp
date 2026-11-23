#include "minidayz/location_event_runtime.hpp"
#include "minidayz/stage1_data_access.hpp"
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
                // Verify original source frame, size, opacity, layer and
                // angle actions never corrupt generated house placements.
                for(const auto& spawn:branch_runtime.spawns()){
                    assert(spawn.object<stage1::kObjectCount);
                    assert(!spawn.layer.empty());
                    assert(std::isfinite(spawn.x)&&std::isfinite(spawn.y));
                    assert(std::isfinite(spawn.angle));
                    assert(std::isfinite(spawn.opacity) &&
                           spawn.opacity>=0.0 && spawn.opacity<=1.0);
                    assert(spawn.frame>=0);
                    assert(std::isfinite(spawn.width)&&
                           std::isfinite(spawn.height));
                    // The root Sprite218 category belongs to the exact
                    // generated marker instance, not a global last value.
                    if (spawn.object == 218) {
                        const auto category =
                            spawn.instance_variables[0].as_number();
                        assert(std::isfinite(category));
                        assert(category >= 0.0);
                    }
                }
                ++executions;
            }
        }
    }
    // 62 authored branches, all tile-code variants, across all six source levels.
    assert(executions >= stage1::kLocationTemplateSourceCount * 6);
}

void test_original_location_spawns_preserved_across_cells() {
    // The original Mapgen creates independent instances for each cell.
    // Destroying a temporary Sprite915 while processing the next cell
    // must never erase the prior cell's scenery or spawn markers.
    construct::Array map;
    assert(map.set_size(16,16,1));
    std::mt19937 rng(0x4d444159u);
    construct::SourceLocationEventRuntime runtime;
    assert(runtime.initialize(0,map,rng));
    const auto* first=find_source_location_template(50);
    assert(first);
    runtime.set_mapgen(50,3,4,1000.0,2000.0,1020.0,1020.0);
    bool matched=false;
    assert(runtime.execute_branch(first->event_uid,matched) && matched);
    const auto previous=runtime.spawns();
    assert(!previous.empty());
    for(const auto code:{9,4,16,17,6,21,52}) {
        const auto* next=find_source_location_template(code);
        assert(next);
        runtime.set_mapgen(code,5,6,3500.0,4700.0,1020.0,1020.0);
        assert(runtime.execute_branch(next->event_uid,matched));
        assert(matched);
        assert(runtime.spawns().size()>=previous.size());
        for(std::size_t i=0;i<previous.size();++i) {
            const auto& before=previous[i];
            const auto& after=runtime.spawns()[i];
            assert(before.object==after.object);
            assert(before.layer==after.layer);
            assert(before.x==after.x && before.y==after.y);
            assert(before.frame==after.frame);
        }
    }
}

void test_root_original_settlement_array_rows() {
    // Root generate_location 9/4/16/17/6/21 branches author many Array525
    // columns; ensure the native interpreter retains their distinct entries.
    // This is the source grouping the player expects on the world map.
    for(const auto code:{9,4,16,17,6,21}){
        const auto* t=minidayz::find_source_location_template(code);
        assert(t);
        minidayz::construct::Array map;
        assert(map.set_size(16,16,1));
        std::mt19937 rng(0x4d444159u+code);
        minidayz::construct::SourceLocationEventRuntime rt;
        assert(rt.initialize(0,map,rng));
        rt.set_mapgen(code,4,5,3760.0,4400.0,1020.0,1020.0);
        bool matched=false;
        assert(rt.execute_branch(t->event_uid,matched) && matched);
        const auto& array=rt.location_array();
        std::size_t count=0,houses=0;
        for(int col=0;col<static_cast<int>(array.width());++col){
            const int cat=static_cast<int>(array.at(col,0).as_number());
            if(cat>0){
                ++count;
                if(cat==7)++houses;
                if(count<=22)
                    std::fprintf(stderr,
                     "ROOT SETTLEMENT ENTRY tile=%d col=%d category=%d variant=%g worldX=%g worldY=%g\n",
                     code,col,cat,array.at(col,1).as_number(),
                     array.at(col,2).as_number(),array.at(col,3).as_number());
            }
        }
        std::fprintf(stderr,
            "ROOT SETTLEMENT ARRAY tile=%d source_uid=%llu rows=%zu count=%zu houses=%zu\n",
            code,static_cast<unsigned long long>(t->event_uid),
            array.height(),count,houses);
        // A village/city source template has multiple authored houses,
        // not just one. Check the live interpreter did not overwrite all
        // original Array525.SetAtXY records at index At(0).
        assert(count>=4);
        assert(houses>=2);
    }
}

} // namespace

int main() {
    test_original_location_spawns_preserved_across_cells();
    test_root_original_settlement_array_rows();
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
