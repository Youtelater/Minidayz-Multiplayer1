#include "minidayz/stage1_data_access.hpp"
#include "minidayz/world.hpp"
#include "minidayz/npc_system.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>

namespace {

std::size_t count_code(const minidayz::WorldGrid& grid, std::uint16_t code) {
    std::size_t count = 0;
    for (int y = 0; y < grid.height(); ++y)
        for (int x = 0; x < grid.width(); ++x)
            if (grid.at(x, y) == code) ++count;
    return count;
}

void test_grid_semantics() {
    minidayz::WorldGrid grid;
    assert(grid.resize(4, 4));
    assert(grid.at(-1, 0) == 0);
    assert(grid.at(4, 0) == 0);
    assert(grid.at(0, 4) == 0);
    assert(grid.set(1, 1, 9));
    assert(!grid.has_empty_3x3(1, 1));
    assert(grid.has_empty_3x3(3, 3));
    assert(!grid.set(4, 4, 1));
}

void test_source_profiles() {
    minidayz::WorldGenerationProfile p{};

    assert(minidayz::source_world_profile(0, p));
    assert(p.map_size_x == 16 && p.map_size_y == 16 && p.grid_height == 16);
    assert(p.villages == 20 && p.militaries == 0 && p.cities == 3);
    assert(p.hospitals == 1 && p.firestations == 1 && p.chaoshi == 0);
    assert(p.gas_stations == 1 && p.roadside_cafes == 1);

    assert(minidayz::source_world_profile(1, p));
    assert(p.villages == 15 && p.militaries == 2 && p.cities == 5);
    assert(p.hospitals == 2 && p.firestations == 2 && p.chaoshi == 1);
    assert(p.gas_stations == 2 && p.lite_shop == 1 && p.roadside_cafes == 2);

    assert(minidayz::source_world_profile(2, p));
    assert(p.villages == 6 && p.militaries == 4 && p.cities == 10);
    assert(p.hospitals == 3 && p.firestations == 3 && p.chaoshi == 1);

    assert(minidayz::source_world_profile(3, p));
    assert(p.villages == 3 && p.militaries == 5 && p.cities == 11);
    assert(p.hospitals == 4 && p.firestations == 2 && p.chaoshi == 1);

    assert(minidayz::source_world_profile(4, p));
    assert(p.grid_height == 33 && p.second_half_start_y == 18);
    assert(p.villages == 4 && p.militaries == 8 && p.cities == 6);
    assert(p.hospitals == 4 && p.firestations == 4 && p.chaoshi == 1);
    assert(p.second_half_villages == 4 && p.second_half_militaries == 6);
    assert(p.second_half_cities == 3 && p.second_half_hospitals == 2);
    assert(p.second_half_firestations == 2 && p.second_half_chaoshi == 2);

    assert(minidayz::source_world_profile(5, p));
    assert(p.grid_height == 33 && p.second_half_start_y == 18);
    assert(p.villages == 3 && p.militaries == 6 && p.cities == 3);
    assert(p.hospitals == 2 && p.firestations == 2 && p.chaoshi == 2);
    assert(!minidayz::source_world_profile(255, p));
}

void test_original_map_layout_and_descriptors() {
    minidayz::WorldSystem world;
    constexpr std::uint32_t seed = 0x4d444159u;
    assert(world.generate_from_source(seed, "Map", 0));
    assert(world.initialized());
    assert(world.seed() == seed);
    assert(world.bounds().width == minidayz::WorldSystem::kSourceMapWidth);
    assert(world.bounds().height == minidayz::WorldSystem::kSourceMapHeight);

    // These counts come from the original Map layout and source spawn calls.
    assert(world.placement_count() >= 1250);
    // The runtime executes the original location-event branches, so spawn
    // counts are derived from their conditions/actions rather than the legacy
    // pre-expanded System Create shortcut table.
    assert(world.source_location_spawn_count() > 0);
    assert(world.placement_count() == 1250 + world.source_location_spawn_count());
    // Includes the 103 authored Map-layout buildings plus buildings instantiated by generated POIs.
    assert(world.feature_count(minidayz::WorldFeatureKind::Building) > 103);
    assert(world.feature_count(minidayz::WorldFeatureKind::SpawnPoint) > 0);
    assert(world.feature_count(minidayz::WorldFeatureKind::Bunker) == 136);
    assert(world.feature_count(minidayz::WorldFeatureKind::Bridge) == 59);

    assert(world.grid().width() == 16 && world.grid().height() == 16);
    assert(count_code(world.grid(), 9) == 20);
    assert(count_code(world.grid(), 5) == 0);
    assert(count_code(world.grid(), 6) == 3);
    assert(count_code(world.grid(), 11) == 1);
    assert(count_code(world.grid(), 15) == 1);
    assert(world.map_marker_count() == 16u * 16u);

    const auto* first = world.map_marker(0);
    assert(first != nullptr);
    assert(world.map_marker(world.map_marker_count()) == nullptr);

    bool found_bunker = false;
    bool found_bridge = false;
    for (std::size_t i = 0; i < world.feature_count(); ++i) {
        const auto* f = world.feature(i);
        assert(f != nullptr);
        if (f->kind == minidayz::WorldFeatureKind::Bunker) {
            assert(world.bounds().contains(f->position));
            found_bunker = true;
        }
        if (f->kind == minidayz::WorldFeatureKind::Bridge) {
            assert(world.bounds().contains(f->position));
            found_bridge = true;
        }
    }
    assert(found_bunker && found_bridge);

    // Spawn-point object 216 is not the visible Zed. Native actors must use
    // the original skin/eyes pair created by Spawn_NPC.
    std::size_t source_zed_points = 0;
    for (std::size_t i = 0; i < world.placement_count(); ++i) {
        const auto* placement = world.placement(i);
        if (placement && placement->object == 216) ++source_zed_points;
    }
    NpcSystem npcs;
    assert(npcs.initialize(world));
    assert(npcs.count() == source_zed_points);
    assert(npcs.count() > 0);
    for (std::size_t i = 0; i < npcs.count(); ++i) {
        const auto* actor = npcs.actor(i);
        assert(actor && actor->alive);
        assert(actor->spawn_source_object == 216);
        assert(actor->source_object == 206);
        assert(actor->eyes_object == 256);
    }
}

void test_seed_reproducibility() {
    minidayz::WorldSystem a, b;
    assert(a.initialize(123456u));
    assert(b.initialize(123456u));
    assert(a.generate_procedural_grid(2, 123456u));
    assert(b.generate_procedural_grid(2, 123456u));
    assert(a.build_map_markers(2, 123456u));
    assert(b.build_map_markers(2, 123456u));
    assert(a.grid().width() == b.grid().width());
    assert(a.grid().height() == b.grid().height());
    assert(a.map_marker_count() == b.map_marker_count());
    for (int y = 0; y < a.grid().height(); ++y)
        for (int x = 0; x < a.grid().width(); ++x)
            assert(a.grid().at(x, y) == b.grid().at(x, y));
    for (std::size_t i = 0; i < a.map_marker_count(); ++i) {
        const auto* lhs = a.map_marker(i);
        const auto* rhs = b.map_marker(i);
        assert(lhs && rhs);
        assert(lhs->cell_x == rhs->cell_x && lhs->cell_y == rhs->cell_y);
        assert(lhs->source_code == rhs->source_code);
        assert(lhs->marker_type == rhs->marker_type);
    }
}

void test_every_source_level_quota() {
    for (std::uint8_t level = 0; level <= 5; ++level) {
        minidayz::WorldGenerationProfile profile{};
        assert(minidayz::source_world_profile(level, profile));

        minidayz::WorldSystem world;
        const std::uint32_t seed = 0x4d445a33u + level * 7919u;
        assert(world.initialize(seed));
        assert(world.generate_procedural_grid(level, seed));
        assert(world.build_map_markers(level, seed));

        const auto total = [](std::uint8_t first, std::uint8_t second) {
            return static_cast<std::size_t>(first) + second;
        };
        assert(count_code(world.grid(), 9) == total(profile.villages, profile.second_half_villages));
        assert(count_code(world.grid(), 5) == total(profile.militaries, profile.second_half_militaries));
        assert(count_code(world.grid(), 6) == total(profile.cities, profile.second_half_cities));
        assert(count_code(world.grid(), 11) == total(profile.hospitals, profile.second_half_hospitals));
        assert(count_code(world.grid(), 15) == total(profile.firestations, profile.second_half_firestations));
        assert(count_code(world.grid(), 103) == total(profile.chaoshi, profile.second_half_chaoshi));
        assert(count_code(world.grid(), 99) == profile.gas_stations);
        assert(count_code(world.grid(), 51) == profile.lite_shop);
        assert(count_code(world.grid(), 50) == profile.roadside_cafes);
        assert(world.map_marker_count() ==
               static_cast<std::size_t>(profile.map_size_x) * profile.grid_height);
    }
}

void test_level_four_split_grid() {
    minidayz::WorldSystem world;
    assert(world.initialize(0x1234u));
    assert(world.generate_procedural_grid(4, 0x1234u));
    assert(world.build_map_markers(4, 0x1234u));
    assert(world.grid().width() == 16 && world.grid().height() == 33);

    assert(count_code(world.grid(), 9) == 8);
    assert(count_code(world.grid(), 5) == 14);
    assert(count_code(world.grid(), 6) == 9);
    assert(count_code(world.grid(), 11) == 6);
    assert(count_code(world.grid(), 15) == 6);
    assert(count_code(world.grid(), 103) == 3);

    // The source uses check_Y 1..15 and 18..32, then stores at Y - 1.
    // Therefore actual rows 15 and 16 remain the two-row gap.
    for (int x = 0; x < world.grid().width(); ++x) {
        assert(world.grid().at(x, 15) == 0);
        assert(world.grid().at(x, 16) == 0);
    }
}

} // namespace

int main() {
    test_grid_semantics();
    test_source_profiles();
    test_original_map_layout_and_descriptors();
    test_seed_reproducibility();
    test_level_four_split_grid();
    test_every_source_level_quota();
    std::puts("World source regression tests passed.");
    return 0;
}
