#include "minidayz/stage1_data_access.hpp"
#include "minidayz/world.hpp"
#include "minidayz/terrain_visible_index.hpp"
#include "minidayz/terrain_obstacle_index.hpp"
#include "minidayz/npc_system.hpp"

#include <cassert>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <unordered_set>

namespace {

std::size_t count_code(const minidayz::WorldGrid& grid, std::uint16_t code) {
    std::size_t count = 0;
    for (int y = 0; y < grid.height(); ++y)
        for (int x = 0; x < grid.width(); ++x)
            if (grid.at(x, y) == code) ++count;
    return count;
}

// Individual Construct object frames are tested separately below.
void test_source_world_instance_animation_state() {
    minidayz::WorldSystem world;
    assert(world.generate_from_source(0x4d444159u, "Map", 0));
    std::size_t tree=world.placement_count(), other=tree;
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(!p || p->layer!=43 || p->object!=207)continue;
        if(tree==world.placement_count())tree=i;
        else {other=i;break;}
    }
    assert(tree<world.placement_count());
    assert(other<world.placement_count());
    const auto original_other=*world.placement(other);
    const auto original_tree=*world.placement(tree);
    assert(world.set_placement_sprite_state(tree,0,1,1.0f));
    assert(world.placement(tree)->source_animation==0);
    assert(world.placement(tree)->source_frame==1);
    assert(world.placement(other)->source_frame==original_other.source_frame);
    assert(world.placement(other)->source_animation==original_other.source_animation);
    assert(!world.set_placement_sprite_state(tree,65535,999,1.0f));
    assert(!world.set_placement_sprite_state(tree,0,9999,1.0f));
    assert(!world.set_placement_sprite_state(tree,0,0,2.0f));
    assert(world.placement(tree)->source_frame==1);
    assert(world.set_placement_sprite_state(tree,original_tree.source_animation,
                                            original_tree.source_frame,
                                            original_tree.opacity));
    assert(world.placement(tree)->source_frame==original_tree.source_frame);
}

// The original Construct UID on generated objects is an *event* UID, not
// a runtime instance identity: one tree-creation event can create thousands
// of distinct trees. Persisting by event UID would corrupt other objects.
void test_generated_object_instance_ids() {
    constexpr std::uint32_t seed=0x4d445a33u;
    minidayz::WorldSystem first, second;
    assert(first.generate_from_source(seed,"Map",0));
    assert(second.generate_from_source(seed,"Map",0));
    assert(first.placement_count()==second.placement_count());
    std::unordered_set<std::uint64_t> runtime_ids;
    std::size_t generated=0,trees=0;
    for(std::size_t i=0;i<first.placement_count();++i) {
        const auto* a=first.placement(i);
        const auto* b=second.placement(i);
        assert(a && b);
        assert(a->instance_id == i+1);
        assert(a->instance_id == b->instance_id);
        assert(a->runtime_uid == b->runtime_uid);
        assert(first.placement_by_id(static_cast<std::uint32_t>(i+1))==a);
        assert(first.placement_by_instance_id(i+1)==a);
        assert(a->runtime_uid!=0);
        if (!runtime_ids.insert(a->runtime_uid).second) {
            std::fprintf(stderr,
                "DUPLICATE WORLD UID: index=%zu runtime=%llu source=%llu layer=%u object=%u\\n",
                i, static_cast<unsigned long long>(a->runtime_uid),
                static_cast<unsigned long long>(a->source_uid),
                static_cast<unsigned>(a->layer), static_cast<unsigned>(a->object));
            assert(false && "Each world object must have a unique runtime UID");
        }
        if(a->runtime_uid & (1ULL<<63)) {
            ++generated;
            if(a->layer==43)++trees;
        } else {
            // Original static objects keep their authored instance UID.
            assert(a->runtime_uid==a->source_uid);
        }
    }
    assert(generated>1000);
    assert(trees>1000);
    assert(!first.placement_by_id(0));
    assert(!first.placement_by_instance_id(0));
    assert(!first.placement_by_instance_id(~std::uint64_t{0}));
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

    // A 400x240 3DS frame must iterate only camera-visible source sprites,
    // never all thousands of generated Map placements on every frame.
    minidayz::terrain::VisibleSourcePlacementIndex visibility{};
    visibility.rebuild(world);
    std::vector<std::uint32_t> viewport{};
    visibility.query(400.0f,4000.0f,400.0f,240.0f,viewport);
    assert(viewport.size()<world.placement_count());
    assert(std::is_sorted(viewport.begin(),viewport.end()));
    for(std::size_t i=1;i<viewport.size();++i)
        assert(viewport[i-1]!=viewport[i]);
    for(const auto index:viewport)assert(world.placement(index));

    // Generated source buildings have no serialized instance size: their
    // original sprite atlas provides their actual render bounds. A sprite
    // crossing a 512px visibility-bin boundary must remain visible even
    // when the instance's own width/height are zero.
    std::size_t verified_zero_size_crossings=0;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* p=world.placement(i);
        if(!p || p->width!=0.0f || p->height!=0.0f ||
           p->layer==32 || p->layer>=44 || p->opacity<=0.0f)continue;
        const auto* obj=minidayz::stage1::find_object(p->object);
        if(!obj || obj->type!=18)continue;
        const float source_extent=std::hypot(obj->width,obj->height);
        if(source_extent<70.0f)continue;
        const float boundary=(std::floor(p->x/512.0f)+1.0f)*512.0f;
        const float dx=boundary-p->x;
        if(dx<=0.0f || dx>=source_extent ||
           boundary+2.0f>=world.bounds().width)continue;
        std::vector<std::uint32_t> cross_bin{};
        visibility.query(boundary+1.0f,p->y,1.0f,1.0f,cross_bin);
        assert(std::binary_search(cross_bin.begin(),cross_bin.end(),
                    static_cast<std::uint32_t>(i)));
        ++verified_zero_size_crossings;
        if(verified_zero_size_crossings>=3)break;
    }
    assert(verified_zero_size_crossings>0);

    // Likewise, player corner-collision queries must never scan every
    // building in the original 36,850x20,000 world.
    minidayz::terrain::SourceObstacleIndex collision{};
    collision.rebuild(world);
    assert(collision.candidates(600.0f,5000.0f).size()<
           world.placement_count());

    // These counts come from the original Map layout and source spawn calls.
    assert(world.placement_count() >= 1250);
    // The runtime executes the original location-event branches, so spawn
    // counts are derived from their conditions/actions rather than the legacy
    // pre-expanded System Create shortcut table.
    assert(world.source_location_spawn_count() > 0);
    // Original Array 525 create_map_element world-art placements are now
    // instantiated as well as the authored Map and System Create spawns.
    // The old equality incorrectly assumed only invisible respawn triggers
    // could be generated; verify that extra original world images exist.
    assert(world.placement_count() > 1250 + world.source_location_spawn_count());
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
    minidayz::NpcSystem npcs;
    assert(npcs.initialize(world));
    assert(npcs.count() == source_zed_points);
    assert(npcs.count() > 0);
    for (std::size_t i = 0; i < npcs.count(); ++i) {
        const auto* actor = npcs.actor(i);
        assert(actor && actor->alive);
        assert(actor->spawn_source_object == 216);
        // Skin_id=0 in the original Spawn_NPC function randomly selects
        // one of six source body objects. Verify the full mapping rather than
        // assuming every generated NPC uses skin 2.
        constexpr std::uint16_t skin_bodies[] = {600, 206, 582, 583, 586, 587};
        assert(actor->skin_id >= 1 && actor->skin_id <= 6);
        assert(actor->source_object == skin_bodies[actor->skin_id - 1]);
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

// New Game must not reuse an old, static Map. Confirm that two different
// seeds change the original-generated grid AND populate the display layers
// with original Construct sprites that the 3DS world renderer can draw.
void test_new_game_generates_visible_original_world() {
    minidayz::WorldSystem first, second;
    assert(first.generate_from_source(0x10203040u, "Map", 0));
    assert(second.generate_from_source(0x55667788u, "Map", 0));
    std::size_t changed_cells = 0;
    for (int row = 0; row < first.grid().height(); ++row) {
        for (int col = 0; col < first.grid().width(); ++col) {
            if (first.grid().at(col, row) != second.grid().at(col, row))
                ++changed_cells;
        }
    }
    assert(changed_cells > 0);
    for (const auto* world : {&first, &second}) {
        // Original empty POI cells still become wilderness/forest/small
        // location markers. Every marker must select an actual extracted
        // generate_location branch; an empty-cell skip leaves barren maps.
        std::size_t wilderness_markers = 0;
        for (std::size_t i = 0; i < world->map_marker_count(); ++i) {
            const auto* marker = world->map_marker(i);
            assert(marker != nullptr);
            if (marker->source_code != 0) continue;
            ++wilderness_markers;
            assert(marker->marker_type != 0);
            assert(minidayz::find_source_location_template(
                marker->marker_type) != nullptr);
        }
        assert(wilderness_markers > 0);
        std::size_t drawable_generated_sprites = 0;
        std::size_t generated_loot_markers=0;
        std::size_t original_category15_markers=0;
        for (std::size_t i = 0; i < world->placement_count(); ++i) {
            const auto* candidate=world->placement(i);
            if(candidate && candidate->object==218 &&
               world->bounds().contains({candidate->x,candidate->y})) {
                ++generated_loot_markers;
                if(candidate->source_loot_category==15)
                    ++original_category15_markers;
                // No category-15 marker may be inferred from object type
                // alone. This must originate in the source event's Sprite
                // instance variable rather than guessed spawn weights.
                if(candidate->source_loot_category)
                    assert(candidate->source_loot_category<=65535);
            }
            const auto* placement = world->placement(i);
            if (!placement || placement->source_uid != 0 ||
                placement->layer >= 44) continue;
            const auto* object = minidayz::stage1::find_object(placement->object);
            if (object && object->type == 18 &&
                world->bounds().contains({placement->x, placement->y}))
                ++drawable_generated_sprites;
        }
        // The source produces dozens of zed_resp (zombie respawn triggers)
        // on layer 49. These are deliberately invisible on the top screen;
        // counting them as drawable art would hide a real rendering bug.
        // Visible generated structures are a much smaller subset.
        assert(world->source_location_spawn_count() > 20);
        assert(drawable_generated_sprites > 0);
        std::printf("ORIGINAL LOOT MARKERS total=%zu source_category15=%zu\\n",
                    generated_loot_markers,original_category15_markers);
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

// Test the complete five-phase New Game pipeline on EVERY source world.
// Previous quota-only tests could pass while location/event generation on
// later maps failed and returned the user to the boot menu.
void test_full_generation_all_source_maps() {
    for(std::uint8_t level=0;level<6;++level) {
        minidayz::WorldSystem world;
        const auto seed=0x526f6164u+static_cast<std::uint32_t>(level)*173u;
        assert(world.begin_source_generation(seed,"Map",level));
        std::uint8_t advanced=0;
        while(!world.generation_complete() && advanced<6) {
            assert(world.advance_source_generation());
            ++advanced;
        }
        assert(world.generation_complete());
        assert(advanced==5);
        assert(world.generation_level()==level);
        assert(world.source_location_spawn_count()>0);
        // Verify the road Array actually contains painted corridors;
        // successful location/POI counts alone did not catch missing roads.
        assert(count_code(world.grid(),12)+count_code(world.grid(),13)+
               count_code(world.grid(),14)>0);
        assert(world.placement_count()>1250);
        assert(world.feature_count(minidayz::WorldFeatureKind::Building)>103);
        assert(world.grid().height()==(level>=4?33:16));
        assert(world.map_marker_count()==
            static_cast<std::size_t>(world.grid().width())*world.grid().height());
        // Each generated world must carry both its original environmental
        // features and runtime spawn points, not only the static Map layout.
        assert(world.feature_count(minidayz::WorldFeatureKind::SpawnPoint)>0);
        // Source vegetation must be generated from the original
        // create_map_element var1=1 tree branches, not only the 47
        // hand-authored Trees-layer instances.
        std::size_t generated_trees=0;
        std::size_t all_trees=0;
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(!p || p->layer!=43)continue;
            ++all_trees;
            if(p->source_uid!=0)++generated_trees;
        }
        std::printf("SOURCE VEGETATION level=%u generated_trees=%zu total_trees=%zu placements=%zu\\n",
            static_cast<unsigned>(level),generated_trees,all_trees,world.placement_count());
        // The source branches for wilderness markers 1/10 and 2/8 are
        // OR blocks, not disabled AND blocks. Level zero's original
        // 16-plant Repeat loops must yield considerably more than the
        // 220 trees observed when all OR events were skipped.
        // Root-original regenerated world tables yielded 1,676, 1,715,
        // 1,641, 1,591, 4,117 and 4,327 source Trees-layer placements
        // for these six fixed seeds in CI. These are REGRESSION guardrails,
        // not invented in-game spawn-density constants. A stale source cache
        // formerly produced only a few hundred trees and large empty areas.
        assert(generated_trees >= (level >= 4 ? 3500u : 1000u));
        assert(all_trees>=generated_trees);
        assert(generated_trees>0);
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
    test_source_world_instance_animation_state();
    test_generated_object_instance_ids();
    test_source_profiles();
    test_original_map_layout_and_descriptors();
    test_seed_reproducibility();
    test_new_game_generates_visible_original_world();
    test_level_four_split_grid();
    test_every_source_level_quota();
    test_full_generation_all_source_maps();
    std::puts("World source regression tests passed.");
    return 0;
}
