#include <memory>
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/world.hpp"
#include "minidayz/source_mapgen_geometry.hpp"
#include "minidayz/world_visual_save.hpp"
#include "minidayz/source_world_event_effects.hpp"
#include "minidayz/source_world_sprite_visibility.hpp"
#include "minidayz/source_door_auto.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/source_furniture_interaction.hpp"
#include "minidayz/source_interior_container.hpp"
#include "minidayz/source_box_simple_drops.hpp"
#include "minidayz/original_initial_building_loot.hpp"
#include "minidayz/original_car_trunk.hpp"
#include "minidayz/generated/source_family_members.hpp"
#include "minidayz/generated/source_house_companions.hpp"
#include "minidayz/generated/source_car_wreck_trunks.hpp"
#include "minidayz/generated/category46_source.hpp"
#include "minidayz/generated/source_building_frame_geometry.hpp"
#include "minidayz/generated/world_source_data.hpp"
#include "minidayz/generated/location_event_source.hpp"
#include "minidayz/generated/terrain_scatter_source.hpp"
#include "minidayz/source_location_terrain.hpp"
#include "minidayz/source_special_terrain.hpp"
#include "minidayz/source_tree_fall_rules.hpp"
#include "minidayz/terrain_visible_index.hpp"
#include "minidayz/terrain_obstacle_index.hpp"
#include "minidayz/source_tree_visibility_collision.hpp"
#include "minidayz/source_crash_site_geometry.hpp"
#include "minidayz/source_road_vehicle_geometry.hpp"
#include "minidayz/source_radiation_zone.hpp"
#include "minidayz/npc_system.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <initializer_list>
#include <utility>
#include <unordered_set>
#include <unordered_map>

namespace {

std::size_t count_code(const minidayz::WorldGrid& grid, std::uint16_t code) {
    std::size_t count = 0;
    for (int y = 0; y < grid.height(); ++y)
        for (int x = 0; x < grid.width(); ++x)
            if (grid.at(x, y) == code) ++count;
    return count;
}

void test_original_high_layer_spawn_visibility() {
    // Reproduce three root create_map_element Sprite creations that the
    // old object241/1126-only whitelist silently hid during 3DS rendering.
    minidayz::WorldSystem world;
    assert(world.initialize(0x71417u));
    const struct Case {
        std::uint8_t variant;
        std::uint16_t object;
        std::uint64_t event_uid;
        const char* layer;
        float x;
    } cases[]={
        {15,1417,611509906059529ULL,"night",3000.0f},
        {16,1419,969885855510364ULL,"night",4000.0f},
        {17,1420,635923503739935ULL,"Rain",5000.0f},
    };
    for(const auto& c:cases){
        const std::size_t before=world.placement_count();
        assert(world.expand_map_element(3,c.variant,0,{c.x,3000.0f})>0);
        bool found=false;
        for(std::size_t i=before;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(p && p->object==c.object && p->source_uid==c.event_uid){
                assert(minidayz::world_source::source_world_high_layer_gameplay_art(
                    p->object,c.layer,p->source_uid));
                found=true;
            }
        }
        assert(found && "Original high-layer scene Sprite was never spawned");
    }
    minidayz::terrain::VisibleSourcePlacementIndex visibility;
    visibility.rebuild(world);
    for(const auto& c:cases){
        std::vector<std::uint32_t> near{};
        visibility.query(c.x-100.0f,2900.0f,200.0f,200.0f,near);
        bool indexed=false;
        for(const auto index:near){
            const auto* p=world.placement(index);
            if(p && p->object==c.object && p->source_uid==c.event_uid)
                indexed=true;
        }
        assert(indexed && "Original high-layer Sprite culled before rendering");
    }
}

void test_building_attachment_name_case() {
    using namespace minidayz::world_source;
    for (const auto& point : kSourceHouseImagePoints) {
        if (std::strcmp(point.name, "w1") != 0) continue;
        const auto* lower = source_house_image_point(
            point.object, point.animation, point.frame, "w1", 0);
        const auto* upper = source_house_image_point(
            point.object, point.animation, point.frame, "W1", 0);
        assert(lower && lower == upper);
        return;
    }
    assert(false && "expected an authored w1 building attachment");
}

void test_source_furniture_and_pump() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    const auto frames=stage1::animations_for_object(kFurnitureBed);
    assert(frames.data && frames.size && frames.data[0].frames==2);
    WorldPlacement bed{};
    bed.object=kFurnitureBed;bed.layer=44;bed.width=37;bed.height=21;
    bed.runtime_uid=5;bed.opacity=1.0f;
    assert(!source_furniture_active(bed));
    bed.layer=kFurnitureLayer;
    assert(source_furniture_active(bed));
    assert(source_pump_draw(50.0f,150).consumed==25);
    assert(source_pump_draw(98.0f,70).consumed==2);
    assert(source_pump_draw(50.0f,7).consumed==7);
    assert(!source_pump_draw(100.0f,150).can_use);
    assert(!source_pump_draw(50.0f,0).can_use);
    WorldSystem world;
    assert(world.initialize(0x95549514u));
    assert(world.load_source_layout("Map"));
    const auto start=world.placement_count();
    assert(world.expand_map_element(3,2,0,{4500.0f,4500.0f},0.0)>0);
    std::size_t pump=world.placement_count();
    for(auto i=start;i<world.placement_count();++i)
        if(world.placement(i)->object==kFurniturePump){pump=i;break;}
    assert(pump<world.placement_count());
    const auto* source=world.placement(pump);
    assert(source_furniture_active(*source));
    // Original choose(100,200) produces exactly 100 or 200. The same seed
    // must consume the same uint32 draw on GCC, Clang and devkitARM.
    assert(source->source_variable_0==100 || source->source_variable_0==200);
    WorldSystem replay;
    assert(replay.initialize(0x95549514u));
    assert(replay.load_source_layout("Map"));
    const auto replay_start=replay.placement_count();
    assert(replay.expand_map_element(3,2,0,{4500.0f,4500.0f},0.0)>0);
    bool found_same=false;
    for(auto i=replay_start;i<replay.placement_count();++i) {
        const auto* candidate=replay.placement(i);
        if(candidate->object==kFurniturePump) {
            assert(candidate->source_variable_0==source->source_variable_0);
            found_same=true;
            break;
        }
    }
    assert(found_same);
    assert(world.set_source_pump_water(pump,70));
    assert(world.placement(pump)->source_variable_0==70);
    assert(!world.set_source_pump_water(pump,201));
    assert(!world.set_source_pump_water(world.placement_count(),5));
}

void test_original_interaction_polygon_geometry() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    const WorldPoint concave[]={{-50,-50},{50,-50},{50,-20},
        {-20,-20},{-20,50},{-50,50}};
    // Rectangle bounds alone falsely accept the empty L-shaped corner.
    assert(!source_polygon_overlaps_box(6,[&](std::size_t i){
        return concave[i];},15,15));
    assert(source_polygon_overlaps_box(6,[&](std::size_t i){
        return WorldPoint{concave[i].x+25,concave[i].y+25};
    },15,15));
    const WorldPoint outer[]={{-80,-80},{80,-80},{80,80},{-80,80}};
    assert(source_polygon_overlaps_box(4,[&](std::size_t i){
        return outer[i];},15,15)); // player entirely inside
    const WorldPoint strip[]={{-80,-1},{80,-1},{80,1},{-80,1}};
    assert(source_polygon_overlaps_box(4,[&](std::size_t i){
        return strip[i];},15,15)); // edges cross, no vertex inside
    const WorldPoint distant[]={{20,20},{20,20},{80,20},{80,80},{20,80}};
    assert(!source_polygon_overlaps_box(5,[&](std::size_t i){
        return distant[i];},15,15)); // duplicate vertices don't add contact
    assert(source_polygon_overlaps_box(4,[&](std::size_t i){
        return WorldPoint{-strip[i].y,strip[i].x};
    },15,15)); // rotated, thin mask
}

// Individual Construct object frames are tested separately below.
void test_original_roof_family_and_transparency() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    assert(kSourceRoofMemberPointCount>=48);
    assert(!stage1::source_family_contains(1049,1049));
    WorldSystem world;
    assert(world.generate_from_source(0x4d444159u,"Map",0));
    std::size_t roofs=0,interiors=0,frames=0;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* placement=world.placement(i);
        if(!placement)continue;
        if(stage1::source_family_contains(1041,placement->object))
            ++interiors;
        if(!stage1::source_family_contains(1049,placement->object))
            continue;
        assert(terrain::source_visual_roof_overlay(placement->object));
        ++roofs;
        const auto* frame=source_roof_member_point(
            placement->object,placement->source_animation,placement->source_frame);
        if(!frame)continue;
        ++frames;
        const WorldPoint player{placement->x,placement->y};
        float opacity=0.0f;
        assert(source_roof_opacity_at_player(*placement,player,false,opacity));
        assert(opacity==1.0f); // A roof alone is NOT a house entry.
        assert(source_roof_opacity_at_player(*placement,player,true,opacity));
        const float threshold=source_roof_in_world_y(*placement,*frame);
        const bool actual_overlap=source_player_overlaps_world_sprite(
            *placement,player,frame->origin_x,frame->origin_y);
        assert(opacity==(actual_overlap&&player.y<=threshold?0.25f:1.0f));
    }
    std::printf("SOURCE ROOF MEMBERS roofs=%zu interiors=%zu frames=%zu\\n",
                roofs,interiors,frames);
    assert(roofs>0 && interiors>0 && frames>0);
    // Root event 9848716569903957 changes each picked family-1049
    // roof's layer to Buildings_bg after player leaves. Ensure the
    // world does not accidentally move any unrelated instance.
    const auto* map=stage1::find_layout("Map");
    assert(map);
    const auto map_id=static_cast<std::uint16_t>(
        map-stage1::kLayoutTable);
    std::uint16_t bg=0xffff;
    for(const auto& layer:stage1::kLayerTable)
        if(layer.layout==map_id &&
           std::strcmp(layer.name,"Buildings_bg")==0){bg=layer.index;break;}
    assert(bg!=0xffff);
    std::size_t first=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i)
        if(stage1::source_family_contains(1049,world.placement(i)->object)){
            first=i;break;
        }
    assert(first<world.placement_count());
    const auto old_layer=world.placement(first)->layer;
    const auto other_layer=world.placement(first+1)->layer;
    assert(world.set_placement_layer(first,bg));
    assert(world.placement(first)->layer==bg);
    assert(world.placement(first+1)->layer==other_layer);
    assert(!world.set_placement_layer(first,0xffff));
    assert(world.set_placement_layer(first,old_layer));
}

void test_location_state_gates_existing_placement() {
    minidayz::WorldSystem world;
    assert(world.initialize(12345));
    const minidayz::WorldPoint position{6000.0f, 6000.0f};
    const auto start = world.placement_count();
    assert(world.expand_map_element(4, 42, 0, position, 1.0) == 0);
    assert(world.expand_map_element(4, 42, 0, position, 0.5) == 0);
    assert(world.placement_count() == start);
    assert(world.expand_map_element(4, 42, 0, position, 0.0) == 1);
    const auto* marker = world.placement(start);
    assert(marker && marker->object == 218 && marker->source_uid == 593462046150106ULL);
}

void test_original_house_loot_marker_spawns() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    assert(kSourceHouseLootMarkerCount==153);
    WorldSystem world;
    assert(world.initialize(12345));
    assert(world.load_source_layout("Map"));
    std::size_t tested=0;
    for(unsigned variant=0;variant<256;++variant){
        bool expected=false;
        for(const auto& action:kSourceHouseLootMarkers)
            expected|=action.variant==variant;
        if(!expected)continue;
        const auto first=world.placement_count();
        assert(world.expand_map_element(7,static_cast<std::uint8_t>(variant),
            0,{6000.0f,6000.0f})>0);
        auto next_marker=first;
        for(const auto& action:kSourceHouseLootMarkers){
            if(action.variant!=variant)continue;
            const WorldPlacement* marker=nullptr;
            for(std::size_t i=next_marker;i<world.placement_count();++i){
                const auto* p=world.placement(i);
                if(p->object==218 && p->source_uid==action.action_uid){
                    marker=p;next_marker=i+1;break;
                }
            }
            assert(marker);
            assert(marker->source_loot_category==action.category);
            assert(marker->source_variable_0==action.category);
            assert(marker->source_initial_loot_trigger==action.trigger_spawn);
            const auto* parent=world.placement_by_instance_id(
                marker->parent_runtime_uid&~(std::uint64_t{1}<<63));
            assert(parent && parent->object==action.parent);
            float x=parent->x,y=parent->y;
            const auto* pt=source_house_image_point(parent->object,
                parent->source_animation,parent->source_frame,
                action.point_name,action.point_index);
            if(pt){
                const float angle=parent->angle*0.01745329251994329577f;
                const float dx=(pt->x-pt->origin_x)*parent->width*
                    (parent->scale_x<0.0f?-1.0f:1.0f);
                const float dy=(pt->y-pt->origin_y)*parent->height;
                x+=dx*std::cos(angle)-dy*std::sin(angle);
                y+=dx*std::sin(angle)+dy*std::cos(angle);
            }
            if(action.offset_y)y+=parent->height*0.5f+15.0f;
            assert(std::fabs(marker->x-x)<0.01f);
            assert(std::fabs(marker->y-y)<0.01f);
            ++tested;
        }
    }
    assert(tested==153);
}

void test_original_generated_house_companions() {
    using namespace minidayz;
    assert(world_source::kSourceHouseCompanionSpawnCount>=15);
    WorldSystem world;
    assert(world.initialize(0x31415926u));
    assert(world.load_source_layout("Map"));
    struct House { std::uint8_t variant;std::uint16_t base,roof; };
    // Exact root Spawn action pairs at source UIDs
    // 5307563843618601, 6055506806351951, 4849857520268099,
    // 9738497725914855 and 2950646084128909.
    const House houses[]={
        {15,306,333},{27,302,321},{28,311,322},
        {29,312,323},{30,313,324}
    };
    for(const auto& h:houses) {
        const auto start=world.placement_count();
        assert(world.expand_map_element(7,h.variant,0,
               {4500.0f+h.variant*30.0f,6000.0f})>0);
        const auto end=world.placement_count();
        std::size_t base=end,roof=end,door=end,wall=end,shadow=end;
        for(std::size_t i=start;i<end;++i) {
            const auto* p=world.placement(i);
            if(p->object==h.base)base=i;
            if(p->object==h.roof)roof=i;
            if(p->object==349)door=i;
            if(p->object==346)wall=i;
            if(p->object==347)shadow=i;
        }
        // The old implementation only created the base sprite. The
        // original source also spawns exterior roof, wall trigger,
        // shadow and an automatic door from the roof's imagepoint.
        if(!(base<end&&roof<end&&wall<end&&shadow<end&&door<end)){
            std::fprintf(stderr,
                "HOUSE SOURCE variant=%u base=%zu roof=%zu wall=%zu shadow=%zu door=%zu start=%zu end=%zu\\n",
                static_cast<unsigned>(h.variant),base,roof,wall,shadow,door,start,end);
            for(std::size_t k=start;k<end;++k)
                std::fprintf(stderr,"HOUSE SOURCE spawn object=%u x=%.2f y=%.2f\\n",
                    world.placement(k)->object,world.placement(k)->x,world.placement(k)->y);
        }
        assert(base<end&&roof<end&&wall<end&&shadow<end&&door<end);
        const auto* foundation=world.placement(base);
        const auto* exterior=world.placement(roof);
        // Authoritative root family 1048 is a building foundation,
        // whereas family 1041 is a distinct interior/collision group.
        assert(stage1::source_family_contains(1048,foundation->object));
        assert(stage1::source_family_contains(1049,exterior->object));
        assert(!stage1::source_family_contains(1041,foundation->object));
        assert(exterior->linked_runtime_uid==foundation->runtime_uid);
        assert(exterior->parent_runtime_uid==foundation->runtime_uid);
        assert(world.placement(door)->parent_runtime_uid==
               exterior->runtime_uid);
        assert(world.placement(door)->house_base_runtime_uid==
               foundation->runtime_uid);
        assert(world.placement(wall)->linked_runtime_uid==
               foundation->runtime_uid);
        assert(world.placement(shadow)->linked_runtime_uid==
               foundation->runtime_uid);
        // Original 347 shadow is a sprite on Map layer 9, not a
        // generated dark rectangle. Its atlas frame, source scaling
        // and original companion event opacity are preserved.
        const auto* original_shadow=world.placement(shadow);
        assert(original_shadow->layer==9);
        assert(original_shadow->source_animation==0 &&
               original_shadow->source_frame==0);
        assert(original_shadow->width>0.0f && original_shadow->height>0.0f);
        bool found_shadow_action=false;
        for(const auto& action:world_source::kSourceHouseCompanionSpawns){
            if(action.base_object!=h.base || action.variant!=h.variant ||
               action.object!=347)continue;
            assert(std::fabs(original_shadow->opacity-action.opacity)<0.0001f);
            found_shadow_action=true;
        }
        assert(found_shadow_action);
        assert(world.placement(door)->source_frame==0);
        assert(world.placement(door)->width>0.0f);
        assert(world.placement(door)->height>0.0f);
        // Original source SetAnimation(city3/etc.) must select the
        // correct frame-specific atlas, never an entire uncut door PNG.
        bool found_original_animation=false;
        for(const auto& action:world_source::kSourceHouseCompanionSpawns){
            if(action.base_object!=h.base ||
               action.variant!=h.variant || action.object!=349)continue;
            found_original_animation=true;
            assert(world.placement(door)->source_animation==
                   action.animation);
        }
        assert(found_original_animation);
        const auto* doorpoint=world_source::source_house_image_point(
            exterior->object,exterior->source_animation,
            exterior->source_frame,"",3);
        assert(doorpoint!=nullptr);
        const float expected_x=exterior->x+
            (doorpoint->x-doorpoint->origin_x)*exterior->width;
        const float expected_y=exterior->y+
            (doorpoint->y-doorpoint->origin_y)*exterior->height;
        assert(std::fabs(world.placement(door)->x-expected_x)<0.05f);
        assert(std::fabs(world.placement(door)->y-expected_y)<0.05f);
    }
}

void test_every_source_mapped_house_spawn_chain() {
    using namespace minidayz;
    WorldSystem world;
    assert(world.initialize(0x4d444159u));
    assert(world.load_source_layout("Map"));
    std::size_t checked=0,created=0;
    // Source-authored (base Sprite,var2) pairs from root Construct
    // SpawnAnotherObject actions. A row's exact event UID may differ from
    // stale precomputed world source headers; compare the real object and
    // original per-branch instance-variable predicate instead.
    for(const auto& action:world_source::kSourceHouseCompanionSpawns){
        if(action.sequence!=0)continue;
        for(const auto& rule:stage1::kWorldElementSpawnSource){
            if(rule.object!=action.base_object ||
               rule.var1_op!=0 || rule.var1_value!=7 ||
               rule.var2_op!=0 || rule.var2_value!=action.variant)
                continue;
            const std::size_t first=world.placement_count();
            world.expand_map_element(7,action.variant,
                rule.var6_op==0?rule.var6_value:0,
                {7500.0f+static_cast<float>(checked%12)*100.0f,
                 7500.0f+static_cast<float>(checked/12)*100.0f});
            const std::size_t last=world.placement_count();
            assert(last>first);
            // Check that exact action's immediate companion was created;
            // no invented roof/door fallback is allowed.
            bool found_base=false,found_child=false;
            for(std::size_t i=first;i<last;++i){
                const auto* p=world.placement(i);
                found_base |= p->object==action.base_object;
                found_child |= p->object==action.object;
            }
            if(!found_base || !found_child)
                std::fprintf(stderr,
                    "SOURCE HOUSE SPAWN MISSING base=%u variant=%u child=%u event=%llu first=%zu last=%zu\\n",
                    action.base_object,action.variant,action.object,
                    static_cast<unsigned long long>(action.event_uid),
                    first,last);
            assert(found_base&&found_child);
            // Every *source original action*, including secondary and
            // tertiary SpawnAnotherObject sprites, needs its own final
            // atlas animation/frame and visible placement. Testing
            // only the first roof missed wall 346, shadow 347 and door.
            for(const auto& child:world_source::kSourceHouseCompanionSpawns){
                assert(child.action_uid!=0 &&
                       "Original source companion must be an action, not only an event");
                if(child.base_object!=action.base_object ||
                   child.variant!=action.variant)continue;
                bool found=false;
                for(std::size_t j=first;j<last;++j){
                    const auto* p=world.placement(j);
                    if(p->object==child.object &&
                       p->source_uid==child.action_uid &&
                       p->source_animation==child.animation &&
                       p->source_frame==child.frame){
                        // Exact literal original post-Spawn effects must
                        // apply to the SAME instance, not its entire
                        // Construct family or a neighboring building.
                        assert(std::fabs(p->opacity-child.opacity)<0.0001f);
                        if(child.angle_degrees!=99999.0f)
                            assert(std::fabs(p->angle-
                                child.angle_degrees)<0.001f);
                        // Original 346 wall variables, 347 shadow UID,
                        // door 349 parent UID and object-218 loot marker
                        // categories are all attached to THAT child.
                        for(const auto& setting:
                            world_source::kSourceHouseCompanionVariables){
                            if(setting.event_uid!=child.event_uid ||
                               setting.base_object!=child.base_object ||
                               setting.variant!=child.variant ||
                               setting.sequence!=child.sequence ||
                               setting.child!=child.object)continue;
                            const auto value=setting.slot==0 ?
                                p->source_variable_0:p->source_variable_1;
                            if(setting.kind==1 && setting.literal_value>=0)
                                assert(value==static_cast<std::uint64_t>(
                                    setting.literal_value));
                            else if(setting.kind==2) {
                                assert(value!=0);
                                const auto* parent=
                                    world.placement_by_instance_id(
                                        value & ~(std::uint64_t{1}<<63));
                                assert(parent);
                                assert(parent->object==setting.uid_object);
                            }
                        }
                        if(p->object==218 && p->source_variable_0>0)
                            assert(p->source_loot_category==
                                   p->source_variable_0);
                        found=true;break;
                    }
                }
                if(!found)
                    std::fprintf(stderr,
                        "ORIGINAL SOURCE MISSING CHILD variant=%u base=%u child=%u animation=%u frame=%u event=%llu\\n",
                        child.variant,child.base_object,child.object,
                        child.animation,child.frame,
                        static_cast<unsigned long long>(child.event_uid));
                assert(found);
            }
            ++checked;
            created+=last-first;
            break;
        }
    }
    assert(world_source::kSourceHouseCompanionVariableCount>=50);
    assert(checked>=5);
    std::size_t restored_primary=0;
    for(const auto& original:world_source::kSourceHousePrimarySpawns) {
        bool cached=false;
        for(const auto& rule:stage1::kWorldElementSpawnSource) {
            if(rule.object==original.object &&
               rule.var1_op==0 && rule.var1_value==7 &&
               rule.var2_op==0 && rule.var2_value==original.variant){
                cached=true;break;
            }
        }
        if(cached)continue;
        const std::size_t begin=world.placement_count();
        world.expand_map_element(7,original.variant,0,
            {9000.0f+static_cast<float>(restored_primary%10)*100.0f,
             12000.0f+static_cast<float>(restored_primary/10)*100.0f});
        bool created_base=false;
        for(std::size_t i=begin;i<world.placement_count();++i)
            if(world.placement(i)->object==original.object)
                created_base=true;
        if(!created_base)
            std::fprintf(stderr,
                "UNCACHED ORIGINAL HOUSE variant=%u base=%u event=%llu\\n",
                original.variant,original.object,
                static_cast<unsigned long long>(original.event_uid));
        assert(created_base);
        ++restored_primary;
    }
    // Every literal original 508.var2 primary Spawn source record has
    // exactly the Mapgen.var1==7 and single var2 guard (source audit
    // reports zero complex-source primary branches). Exercise ALL
    // source records, including duplicates and the uncached additions,
    // not only the five known city-house designs.
    std::size_t all_original_primary_branches=0;
    for(const auto& original:world_source::kSourceHousePrimarySpawns){
        const auto first=world.placement_count();
        world.expand_map_element(7,original.variant,0,
            {16500.0f+static_cast<float>(all_original_primary_branches%10)*120.0f,
             9000.0f+static_cast<float>(all_original_primary_branches/10)*120.0f});
        const auto last=world.placement_count();
        bool found_base=false;
        for(std::size_t i=first;i<last;++i)
            if(world.placement(i)->object==original.object)
                found_base=true;
        if(!found_base)
            std::fprintf(stderr,
                "UNIMPLEMENTED ORIGINAL HOUSE Spawn record var2=%u object=%u event=%llu\\n",
                original.variant,original.object,
                static_cast<unsigned long long>(original.event_uid));
        assert(found_base);
        for(const auto& child:world_source::kSourceHouseCompanionSpawns){
            if(child.base_object!=original.object ||
               child.variant!=original.variant)continue;
            bool created_child=false;
            for(std::size_t i=first;i<last;++i){
                const auto* p=world.placement(i);
                if(p->object==child.object &&
                   p->source_animation==child.animation &&
                   p->source_frame==child.frame){
                    created_child=true;break;
                }
            }
            if(!created_child)
                std::fprintf(stderr,
                    "ORIGINAL PRIMARY HOUSE CHILD MISSING var2=%u base=%u child=%u\\n",
                    original.variant,original.object,child.object);
            assert(created_child);
        }
        ++all_original_primary_branches;
    }
    // Root house branch UID 304859375570401 (var2=36) literally
    // spawns object 348 frame 30 at parent 1323 ImagePoint 1.
    // Parent 1323 has NO authored custom imagepoints: Construct falls
    // back to its image origin, not to an invented PNG crop position.
    // This previously dropped the original invisible 348 collider.
    const std::size_t missing_point_start=world.placement_count();
    world.expand_map_element(4,36,0,{17123.0f,10123.0f});
    bool source1323=false,source348frame30=false;
    for(std::size_t i=missing_point_start;i<world.placement_count();++i){
        const auto* p=world.placement(i);
        if(p->object==1323)source1323=true;
        if(p->object==348 && p->source_frame==30){
            source348frame30=true;
            assert(std::fabs(p->x-17123.0f)<0.01f);
            assert(std::fabs(p->y-10123.0f)<0.01f);
        }
    }
    assert(source1323&&source348frame30);
    // A category-4 structure must not appear in the category-7 house
    // slot sharing the same variant number.
    const auto other_start=world.placement_count();
    world.expand_map_element(7,36,0,{17323.0f,10123.0f});
    for(auto i=other_start;i<world.placement_count();++i)
        assert(world.placement(i)->object!=1323);
    assert(all_original_primary_branches==
           world_source::kSourceHousePrimarySpawnCount);
    assert(all_original_primary_branches==51);
    // Exhaust all 256 category-7 variant values: each supported root
    // variant must emit exactly one primary sprite with its root UID
    // and unsupported variants must not leak stale cached houses.
    for(unsigned variant=0;variant<256;++variant){
        const auto begin=world.placement_count();
        const auto* expected=static_cast<const world_source::SourceHousePrimarySpawn*>(nullptr);
        for(const auto& r:world_source::kSourceHousePrimarySpawns)
            if(r.variant==variant){assert(!expected);expected=&r;}
        world.expand_map_element(7,static_cast<std::uint8_t>(variant),0,
            {18000.0f,10000.0f});
        if(!expected){assert(world.placement_count()==begin);continue;}
        assert(world.placement_count()>begin);
        const auto* base=world.placement(begin);
        assert(base->object==expected->object);
        // Every one of the ORIGINAL 51 house variants has literal Sprite
        // animation0/frame0 size + hotspot. The compact ObjectDef table
        // stores 0x0 defaults; using those collapses named ImagePoints
        // for all attached roofs, walls and interior sprites at runtime.
        const auto* source_geometry=stage1::source_building_frame_geometry(
            expected->object,0,0);
        assert(source_geometry);
        assert(base->width==source_geometry->width);
        assert(base->height==source_geometry->height);
        assert(base->origin_x==source_geometry->origin_x);
        assert(base->origin_y==source_geometry->origin_y);
        assert(base->width>0.0f && base->height>0.0f);
        assert(std::fabs(base->x-18000.0f)<0.01f);
        assert(std::fabs(base->y-10000.0f)<0.01f);
        std::size_t primary_count=0;
        for(auto k=begin;k<world.placement_count();++k)
            if(world.placement(k)->parent_runtime_uid==0 &&
               world.placement(k)->object==expected->object)++primary_count;
        assert(primary_count==1);
    }
    std::printf("ROOT HOUSE SOURCE SPAWN COVERAGE %zu cached branches %zu restored uncached original branches %zu placed sprites %zu original primary source records checked\\n",
                checked,restored_primary,created,all_original_primary_branches);
}

void test_live_original_automatic_doors() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    assert(kSourceDoorPointCount>=20);
    std::size_t metal=0,ordinary=0;
    for(const auto& point:kSourceDoorPoints){
        assert(point.width>0.0f&&point.height>0.0f);
        assert(std::isfinite(point.in_x)&&std::isfinite(point.in_y));
        if(point.metal) ++metal;
        else ++ordinary;
    }
    assert(metal>0 && ordinary>0);

    WorldSystem world;
    assert(world.generate_from_source(0x4d444159u,"Map",0));
    std::size_t tested=0;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* door=world.placement(i);
        if(!door || door->object!=349 || door->source_frame!=0)
            continue;
        const auto* f=source_door_point(door->source_animation,0);
        if(!f)continue;
        const auto uid=door->runtime_uid;
        const float closed_width=door->width, closed_height=door->height;
        // Each original visual door 349 owns its distinct spawned Solid
        // object 348 on the original Buildings layer, frame 15 at the
        // authored Imagepoint 1 with angle Door.Angle+6.
        assert(door->linked_runtime_uid!=0);
        const auto companion_id=door->linked_runtime_uid &
            ~(std::uint64_t{1}<<63);
        const auto* solid=world.placement_by_instance_id(companion_id);
        assert(solid && solid->object==348);
        assert(solid->runtime_uid==door->linked_runtime_uid);
        assert(solid->linked_runtime_uid==uid);
        // Exact original close event writes 348.UID into 349
        // instance variable 0; a separate instance owns each door.
        assert(door->source_variable_0==solid->runtime_uid);
        assert(solid->source_frame==15);
        assert(solid->opacity==1.0f);
        assert(std::fabs(solid->angle-(door->angle+6.0f))<0.001f);
        // Root open event destroys 348 BEFORE the 0.01s Wait: the door
        // remains frame0 while its collision is already absent.
        assert(world.set_linked_door_solid_present(i,false));
        assert(world.placement(i)->source_frame==0);
        assert(world.placement_by_instance_id(companion_id)->opacity==0.0f);
        assert(world.set_linked_door_solid_present(i,true));
        assert(world.placement_by_instance_id(companion_id)->opacity==1.0f);
        const WorldPoint open_at{door->x,source_door_in_y(*door,*f)+1.0f};
        if(!source_door_overlaps_player(*door,*f,open_at))continue;
        assert(source_auto_door_for_player(world,uid,{1e20f,1e20f})==
               AutomaticDoorEffect::none);
        const auto planned=
            source_auto_door_intended_for_placement(world,i,open_at);
        assert(planned==(f->metal?AutomaticDoorEffect::open_metal:
                                    AutomaticDoorEffect::open_wood));
        assert(world.placement(i)->source_frame==0);
        assert(!source_apply_auto_door_intention(world,i,uid+1,planned));
        assert(world.placement(i)->source_frame==0);
        const auto opened=source_auto_door_for_player(world,uid,open_at);
        assert(opened==(f->metal?AutomaticDoorEffect::open_metal:
                                   AutomaticDoorEffect::open_wood));
        assert(world.placement(i)->source_frame==1);
        if(const auto* next=source_door_point(door->source_animation,1)){
            assert(std::fabs(door->width-closed_width*next->width/f->width)<0.01f);
            assert(std::fabs(door->height-closed_height*next->height/f->height)<0.01f);
            assert(std::fabs(door->origin_x-next->origin_x)<0.001f);
            assert(std::fabs(door->origin_y-next->origin_y)<0.001f);
        }
        assert(world.placement(i)->source_variable_0==solid->runtime_uid);
        assert(world.placement_by_instance_id(companion_id)->opacity==0.0f);
        assert(!source_apply_auto_door_intention(world,i,uid,planned));
        assert(source_auto_door_for_player(world,uid,open_at)==
               AutomaticDoorEffect::none);
        const auto* f1=source_door_point(door->source_animation,1);
        if(f1){
            const WorldPoint close_at{door->x,
                source_door_in_y(*world.placement(i),*f1)-1.0f};
            if(source_door_overlaps_player(*world.placement(i),*f1,close_at)){
                const auto closed=source_auto_door_for_player(
                    world,uid,close_at);
                assert(closed==(f1->metal?AutomaticDoorEffect::close_metal:
                                           AutomaticDoorEffect::close_wood));
                assert(world.placement(i)->source_frame==0);
                const auto* restored=world.placement_by_instance_id(companion_id);
                assert(restored && restored->opacity==1.0f);
                assert(restored->linked_runtime_uid==uid);
                assert(world.placement(i)->source_variable_0==
                       restored->runtime_uid);
                assert(std::fabs(restored->angle-
                    (world.placement(i)->angle+6.0f))<0.001f);
            }
        }
        assert(world.placement(i)->runtime_uid==uid);
        ++tested;
        if(tested>=5)break;
    }
    std::printf("SOURCE LIVE DOORS opened=%zu\\n",tested);
    assert(tested>0);
}

void test_source_tree_fall_item_table() {
    using namespace minidayz::world_source;
    constexpr std::uint16_t objects[]={184,207,209,576,577,581};
    constexpr std::uint16_t expected_count[]={2,3,4,2,3,4};
    constexpr std::uint16_t items[][4]={
        {50,50,0,0}, {38,38,50,0}, {38,38,50,37},
        {50,50,0,0}, {50,50,38,0}, {50,50,38,38}
    };
    for(std::size_t i=0;i<6;++i) {
        const auto* r=tree_fall_rule(objects[i]);
        assert(r && r->count==expected_count[i]);
        assert(r->fall_frame==1 && r->fall_animation==0);
        assert(r->event_uid!=0);
        for(std::size_t j=0;j<4;++j)assert(r->drop_item_ids[j]==items[i][j]);
    }
    assert(tree_fall_rule(349)==nullptr); // Doors do not drop tree resources.
}

// Root Game_events/Hitting_obstacles: branch UID 8754965976446553
// subtracts 1 from instance variable 0 before child fall UID
// 2738244684146745 checks that the new value is <=0.
void test_original_car_trunk_one_time_search() {
    using namespace minidayz;
    using namespace minidayz::items;
    // Root Game_events exact Sprite241 frame-group pools, including
    // duplicates (which are intended weighted choose() entries).
    assert(kOriginalCarTrunkBaseChance==50);
    assert(kOriginalCarTrunkBonusOnTrunkster==10);
    assert(kOriginalCarTrunkExtraChance==10);
    assert(kOriginalCarTrunkDropImagepoint==3);
    assert(kOriginalCarTrunkPools[0].count>0);
    for(std::uint32_t frame=0;frame<6;++frame) {
        const auto* pool=original_car_pool(frame);
        assert(pool && pool->count>0);
        assert(pool->first+pool->count<=
            sizeof(kOriginalCarTrunkItems)/sizeof(kOriginalCarTrunkItems[0]));
        for(std::size_t j=0;j<pool->count;++j)
            assert(source_item_exists(kOriginalCarTrunkItems[pool->first+j]));
        assert(original_car_trunk_item(*pool,0));
    }
    assert(!original_car_pool(6));
    assert(original_car_trunk_chance(0));
    assert(original_car_trunk_chance(0,10));
    assert(!original_car_trunk_chance(0xffffffffu));
    assert(!original_car_trunk_chance(0xffffffffu,10));
    assert(original_car_trunk_extra_chance(0));
    assert(!original_car_trunk_extra_chance(0xffffffffu));
    assert(original_car_pool(0)==original_car_pool(3));
    assert(original_car_pool(3)==original_car_pool(5));
    assert(original_car_pool(2)==original_car_pool(4));
    assert(original_car_pool(1)!=original_car_pool(0));
    assert(original_car_pool(2)!=original_car_pool(1));

    WorldSystem world;
    assert(world.initialize(0xCA241u));
    // Root create_map_element category6 contains the original car wrecks.
    // Category4 is unrelated scenery and legitimately has no car trunks.
    // Exercise every category6 car variant through its source Spawn events.
    for(std::uint8_t variant=1;variant<=20;++variant)
        (void)world.expand_map_element(6,variant,0,
            {14000.0f+variant*350.0f,9000.0f},0.0);
    std::size_t trunk=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(p && p->object==241 && p->opacity>0.0f &&
           original_car_pool(p->source_frame)) {
            trunk=i;
            break;
        }
    }
    // The source has exactly fifteen category6 nested var3=0 trunks.
    // An older parser dropped all fifteen; a second parser accidentally
    // duplicating them would be just as wrong. Test their exact source UID,
    // authored frame and frame-dependent original collision dimensions.
    std::size_t original_trunks=0;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* p=world.placement(i);
        if(!p || p->object!=241)continue;
        ++original_trunks;
        const auto found=std::find_if(
            std::begin(world_source::kSourceCarWreckTrunks),
            std::end(world_source::kSourceCarWreckTrunks),
            [&](const auto& spec) {
                return p->source_uid==spec.source_uid &&
                       p->source_frame==spec.frame &&
                       p->width==spec.width && p->height==spec.height;
            });
        assert(found!=std::end(world_source::kSourceCarWreckTrunks));
    }
    assert(original_trunks==15); // root source variants 1..12,15..17
    assert(trunk<world.placement_count());
    const auto* before=world.placement(trunk);
    assert(before && before->source_variable_0==0);
    // Root 1041.SetFrame(1) affects the parent car BODY, separately from
    // object241.Destroy. A surviving visible frame0 car would be wrong.
    const std::uint64_t body_uid=before->parent_runtime_uid;
    assert(body_uid!=0);
    std::size_t body=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i)
        if(world.placement(i)->runtime_uid==body_uid){body=i;break;}
    assert(body<world.placement_count());
    const auto* car_body=world.placement(body);
    assert(car_body && stage1::source_family_contains(1041,car_body->object));
    // Original wreck Sprite frame dimensions are needed to position its
    // spawned trunk241 at the exact authored imagepoint (not zero-size).
    assert(car_body->width>0.0f && car_body->height>0.0f);
    assert(stage1::source_building_frame_geometry(
        car_body->object,car_body->source_animation,car_body->source_frame));
    assert(car_body->source_frame==0);
    const auto* point=world_source::source_house_image_point(
        before->object,before->source_animation,before->source_frame,"",3);
    assert(point); // original Sprite241 Imagepoint3 on every car frame
    // Root family1041 Overlaps241 gate must be honored for car loot.
    assert(original_car_trunk_has_source_body(world,*before));
    GroundLootSystem ground;
    std::mt19937 random(1u);  // deterministic source sample under 50
    const auto outcome=source_search_original_car_trunk(
        world,trunk,ground,0,0,random);
    assert(outcome==OriginalCarSearchResult::found);
    assert(ground.count()==1);
    const auto* searched=world.placement(trunk);
    assert(searched && searched->source_variable_0==1);
    assert(searched->opacity==0.0f);
    assert(world.placement(body)->source_frame==1);
    assert(source_search_original_car_trunk(world,trunk,ground,0,0,random)
           ==OriginalCarSearchResult::invalid);
    assert(ground.count()==1); // car cannot be searched repeatedly
    const auto* loot=ground.entry(0);
    assert(loot && source_item_exists(loot->item.item_id));
    GroundLootSnapshot snapshot=ground.snapshot();
    GroundLootSystem restored;
    assert(restored.restore(snapshot));
    assert(restored.count()==1);
}


void test_original_radiation_site_world_generation(){
    // Original function wuranqu runs after mapgen_toelements/minimap,
    // helicopter crashes and bunker creation. Source Sprite1126 is
    // visible Rain layer44 (2000px). Type70 overlays are independent
    // of picked source toxic zones.
    for(std::uint8_t level=0;level<6;++level){
        minidayz::WorldSystem world;
        assert(world.generate_from_source(
            0x10203579u+static_cast<std::uint32_t>(level)*79u,
            "Map",level));
        const auto* layout=minidayz::stage1::find_layout("Map");
        assert(layout);
        const auto layout_index=static_cast<std::uint16_t>(
            layout-minidayz::stage1::kLayoutTable);
        std::size_t military=0,picked=0;
        minidayz::terrain::VisibleSourcePlacementIndex visible;
        visible.rebuild(world);
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* zone=world.placement(i);
            if(!zone || zone->object!=
                minidayz::world_source::kOriginalRadioactiveSprite)continue;
            if(zone->source_uid!=
                minidayz::world_source::kOriginalRadioactiveMilitarySpawnAction &&
               zone->source_uid!=
                minidayz::world_source::kOriginalRadioactivePickedSpawnAction)
                continue; // Source authored layout marker is different.
            const auto* layer=minidayz::stage1::find_layer(
                layout_index,zone->layer);
            assert(layer&&std::strcmp(layer->name,"Rain")==0);
            assert(zone->width==2000.0f&&zone->height==2000.0f);
            assert(zone->origin_x==0.5f&&zone->origin_y==0.5f);
            assert(zone->source_frame==0);
            assert(zone->source_variable_0>0 &&
                   zone->source_variable_0<=world.map_marker_count());
            const auto* marker=world.map_marker(
                static_cast<std::size_t>(zone->source_variable_0-1));
            assert(marker);
            const auto p=minidayz::world_source::mapgen_hotspot(
                marker->cell_x,marker->cell_y);
            assert(zone->x==p.x+500.0f&&zone->y==p.y+500.0f);
            std::vector<std::uint32_t> nearby;
            visible.query(zone->x,zone->y,400.0f,240.0f,nearby);
            assert(std::find(nearby.begin(),nearby.end(),
                static_cast<std::uint32_t>(i))!=nearby.end());
            if(zone->source_uid==
                minidayz::world_source::kOriginalRadioactiveMilitarySpawnAction){
                assert(marker->marker_type==70);
                ++military;
            }else{
                assert(minidayz::world_source::original_radioactive_site_type(
                    marker->marker_type));
                ++picked;
            }
        }
        std::size_t military_markers=0;
        for(std::size_t i=0;i<world.map_marker_count();++i)
            if(world.map_marker(i)->marker_type==70)
                ++military_markers;
        assert(military==military_markers);
        const auto per_half=minidayz::world_source::
            original_radioactive_site_quota(level,world.generation_perk_dao()==7);
        const auto half_count=level>=4?2u:1u;
        assert(picked<=static_cast<std::size_t>(per_half)*half_count);
        assert(picked==(level<=2?0u:
                         static_cast<std::size_t>(per_half)*half_count));
        std::fprintf(stderr,
            "SOURCE RADIOACTIVE ZONES level=%u military=%zu picked=%zu\\n",
            level,military,picked);
    }
}

void test_original_bunker_entrance_generation() {
    for(std::uint8_t level=0;level<6;++level){
        minidayz::WorldSystem world;
        assert(world.begin_source_generation(0x7bca12u+level,"Map",level));
        for(int phase=0;phase<4;++phase)assert(world.advance_source_generation());
        std::vector<std::size_t> used_crash_markers;
        std::vector<const minidayz::WorldPlacement*> entrances;
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(p->object==344 && p->source_uid==667245097137285ULL)
                used_crash_markers.push_back(p->source_variable_1);
            if(p->object==434 && (p->source_uid==770992642019829ULL ||
                p->source_uid==515255034690416ULL))
                used_crash_markers.push_back(p->house_base_runtime_uid);
            if(p->object==785 && p->source_uid==644090747484030ULL)
                entrances.push_back(p);
        }
        assert(entrances.size()==(level==0?0u:level<4?1u:2u));
        for(const auto* p:entrances){
            assert(std::find(used_crash_markers.begin(),used_crash_markers.end(),
                p->source_map_marker_index)==used_crash_markers.end());
            assert(p->source_map_marker_index>0);
            const auto* m=world.map_marker(p->source_map_marker_index-1);
            assert(m && (m->marker_type==1 || m->marker_type==10 ||
                         m->marker_type==2 || m->marker_type==8));
            const auto h=minidayz::world_source::mapgen_hotspot(m->cell_x,m->cell_y);
            assert(p->x==h.x+500.0f && p->y==h.y+500.0f);
            assert(p->width==150.0f && p->height==94.0f);
            const bool second=m->cell_y>=17;
            assert(second || m->cell_y<15);
            assert(p->source_frame==(second?2u:0u));
            assert(p->source_variable_2==(second?1u:0u));
        }
    }
    // Explicitly exercise the original 6-per-half radiation variant
    // without relying on the seeded level-five theme choice.
    minidayz::WorldSystem toxic;
    assert(toxic.begin_source_generation(0x10203579u,"Map",5,7));
    for(int phase=0;phase<4;++phase)assert(toxic.advance_source_generation());
    unsigned counts[2]={0,0};
    for(std::size_t i=0;i<toxic.placement_count();++i){
        const auto* p=toxic.placement(i);
        if(p->object!=1126 || p->source_uid!=
            minidayz::world_source::kOriginalRadioactivePickedSpawnAction)continue;
        const auto* m=toxic.map_marker(p->source_variable_0-1);
        assert(m);
        ++counts[m->cell_y>=17?1:0];
    }
    assert(counts[0]==6 && counts[1]==6);
}

void test_original_source_road_vehicle_placement() {
    // Car_respawn_uaz/volga each PickRandom a first-half road12/13,
    // independently of category-6 searchable junk cars. Hummer/KA
    // use their original Mapgen marker70/104 respectively.
    for(std::uint8_t level=0;level<6;++level){
        minidayz::WorldSystem world;
        assert(world.generate_from_source(
            0x246809U+static_cast<std::uint32_t>(level)*311u,
            "Map",level));
        std::size_t uaz=0,volga=0,hammer=0,ka=0;
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(!p)continue;
            if(p->object==809 &&
               (p->source_uid==117612298080658ULL ||
                p->source_uid==812947475463361ULL)){
                ++uaz;
                assert(p->width==72.0f && p->height==32.0f);
                assert(p->angle==0||p->angle==90||
                       p->angle==180||p->angle==270);
                assert(p->linked_runtime_uid!=0);
            }
            if(p->object==815 &&
               (p->source_uid==375484843290557ULL ||
                p->source_uid==613501860905527ULL)){
                ++volga;
                assert(p->width==89.0f && p->height==35.0f);
                assert(p->linked_runtime_uid!=0);
            }
            if(p->object==920 && p->source_uid==579079383275854ULL){
                ++hammer;
                assert(p->angle==180.0f);
            }
            if(p->object==1343 &&
               p->source_uid==117612298080658ULL){
                ++ka;
                assert(p->width==91.0f && p->height==41.0f);
            }
        }
        assert(uaz<=1&&volga<=1&&hammer<=1&&ka<=1);
        // Source System.PickRandom(514) with zero selected instances is
        // a no-op: a seed can leave NO first-half raw road12/13 after
        // gas/cafe/shop replacements. Don't manufacture a vehicle to
        // satisfy a test; match the exact source candidate selection.
        std::size_t available_roads=0,available_hammer=0,available_ka=0;
        for(std::size_t i=0;i<world.map_marker_count();++i){
            const auto* marker=world.map_marker(i);
            assert(marker);
            if(marker->cell_y<=15 &&
               (marker->marker_type==12||marker->marker_type==13))
                ++available_roads;
            if(marker->cell_y<=15 && marker->marker_type==70)
                ++available_hammer;
            if(marker->marker_type==104)++available_ka;
        }
        std::fprintf(stderr,
            "SOURCE ROAD VEHICLES level=%u candidates=%zu uaz=%zu volga=%zu hammer=%zu ka=%zu\\n",
            level,available_roads,uaz,volga,hammer,ka);
        assert(uaz==(available_roads>0?1u:0u));
        assert(volga==(available_roads>0?1u:0u));
        assert(hammer==(available_hammer>0?1u:0u));
        assert(ka==(available_ka>0?1u:0u));
    }
}

void test_original_crash_site_world_generation() {
    // Root generate_helicrashes creates Sprite344 for original
    // {1,2,8,10,52} wilderness and Sprite434 for {5,15,13}.
    // create_map_location then creates an actual Sprite246 helicopter
    // wreck / Sprite441 hammer wreck and the original category 15/18
    // loot markers, not guessed cars from the generic generator.
    for(std::uint8_t level=0;level<6;++level){
        minidayz::WorldSystem world;
        assert(world.generate_from_source(0x4d444159u+level*137u,
            "Map",level));
        std::size_t heli=0,hammer=0,loot15=0,loot18=0;
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(!p)continue;
            if(p->object==344 && p->source_uid==667245097137285ULL){
                assert(p->source_variable_1>0);
                const auto* m=world.map_marker(p->source_variable_1-1);
                assert(m);
                const auto h=minidayz::world_source::mapgen_hotspot(m->cell_x,m->cell_y);
                assert(p->x==h.x+500.0f && p->y==h.y+500.0f);
            }
            if(p->object==434 && (p->source_uid==770992642019829ULL || p->source_uid==515255034690416ULL)){
                assert(p->house_base_runtime_uid>0);
                const auto* m=world.map_marker(p->house_base_runtime_uid-1);
                assert(m);
                const auto h=minidayz::world_source::mapgen_hotspot(m->cell_x,m->cell_y);
                assert(p->x==h.x+500.0f && p->y==h.y+600.0f);
            }
            if(p->object==246 &&
               (p->source_uid==453361015394042ULL ||
                p->source_uid==416326667732045ULL ||
                p->source_uid==439357571825697ULL)){
                ++heli;
                assert(p->width==240.0f&&p->height==186.0f);
                assert(p->parent_runtime_uid!=0);
                std::size_t loot_found=0;
                for(std::size_t j=0;j<world.placement_count();++j){
                    const auto* marker=world.placement(j);
                    if(marker&&marker->object==218&&
                       marker->parent_runtime_uid==p->runtime_uid){
                        assert(marker->source_loot_category==15);
                        assert(marker->source_variable_0==15);
                        assert(marker->source_initial_loot_trigger);
                        ++loot_found;
                    }
                }
                assert(loot_found==3);
            }
            if(p->object==441 &&
               (p->source_uid==832344210313130ULL ||
                p->source_uid==672392095922871ULL)){
                ++hammer;
                assert(p->width>0.0f&&p->height>0.0f);
                assert(p->parent_runtime_uid!=0);
                std::size_t loot_found=0;
                for(std::size_t j=0;j<world.placement_count();++j){
                    const auto* marker=world.placement(j);
                    if(marker&&marker->object==218&&
                       marker->parent_runtime_uid==p->runtime_uid){
                        assert(marker->source_loot_category==18);
                        assert(marker->source_variable_0==18);
                        assert(marker->source_initial_loot_trigger);
                        ++loot_found;
                    }
                }
                assert(loot_found==3);
            }
            if(p->object==218 && p->source_loot_category==15)++loot15;
            if(p->object==218 && p->source_loot_category==18)++loot18;
        }
        const std::size_t halves=level>=4?2u:1u;
        assert(heli==halves*minidayz::world_source::source_helicrash_quota(level));
        assert(hammer<=halves*minidayz::world_source::source_hammercrash_quota(level));
        assert(loot15>=heli*3);
        assert(loot18>=hammer*3);
        std::fprintf(stderr,
            "SOURCE CRASH SITES level=%u heli=%zu hammer=%zu category15=%zu category18=%zu\\n",
            level,heli,hammer,loot15,loot18);
    }
}

void test_original_source_settlement_building_groups() {
    // Group original category-7 primary house instances by the *source*
    // Mapgen514 1020px cell, not by a guessed density multiplier.
    // This verifies that a town consists of multiple independent buildings
    // rather than one building texture painted over a location marker.
    minidayz::WorldSystem world;
    assert(world.generate_from_source(0x4d444159u,"Map",0));
    std::size_t groups=0, clustered=0, largest=0;
    const auto& grid=world.grid();
    for(int y=0;y<grid.height();++y)
        for(int x=0;x<grid.width();++x){
            const auto kind=grid.at(x,y);
            if(kind!=9 && kind!=6)continue;
            const auto center=minidayz::world_source::mapgen_hotspot(x,y);
            std::size_t foundations=0;
            for(std::size_t i=0;i<world.placement_count();++i){
                const auto* p=world.placement(i);
                if(!p||std::fabs(p->x-center.x)>510.0f ||
                    std::fabs(p->y-center.y)>510.0f)continue;
                for(const auto& root:minidayz::world_source::kSourceHousePrimarySpawns)
                    if(p->source_uid==root.event_uid &&
                       p->object==root.object){
                        ++foundations;break;
                    }
            }
            ++groups;
            if(foundations>=2)++clustered;
            largest=std::max(largest,foundations);
        }
    std::fprintf(stderr,
        "ROOT SETTLEMENT GROUPS seed=4d444159 towns=%zu multi_building=%zu largest=%zu\n",
        groups,clustered,largest);
    std::size_t total_source_houses=0;
    float min_center_distance=std::numeric_limits<float>::infinity();
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* placement=world.placement(i);
        if(!placement)continue;
        bool house=false;
        for(const auto& source:minidayz::world_source::kSourceHousePrimarySpawns)
            if(placement->source_uid==source.event_uid &&
               placement->object==source.object){house=true;break;}
        if(!house)continue;
        ++total_source_houses;
        float best=std::numeric_limits<float>::infinity();
        for(int y=0;y<grid.height();++y)
            for(int x=0;x<grid.width();++x){
                if(grid.at(x,y)!=9&&grid.at(x,y)!=6)continue;
                const auto c=minidayz::world_source::mapgen_hotspot(x,y);
                best=std::min(best,std::max(std::fabs(placement->x-c.x),
                                             std::fabs(placement->y-c.y)));
            }
        min_center_distance=std::min(min_center_distance,best);
        if(total_source_houses<=12)
            std::fprintf(stderr,
                "ROOT HOUSE POS object=%u src=%llu x=%.1f y=%.1f nearest_town=%.1f\n",
                static_cast<unsigned>(placement->object),
                static_cast<unsigned long long>(placement->source_uid),
                placement->x,placement->y,best);
    }
    std::fprintf(stderr,"ROOT GROUP DIAGNOSTIC source_house_total=%zu nearest_town=%.1f\n",
        total_source_houses,min_center_distance);
    assert(groups>0 && total_source_houses>0);
    assert(clustered>0 && largest>=2);

    // Verify the REAL procedural generator, not only a manually expanded
    // category-6 fixture. Its original car-wreck category must create
    // searchable Sprite241 markers linked to the actual family1041 body.
    std::size_t generated_car_trunks=0;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* marker=world.placement(i);
        if(!marker || marker->object!=241 ||
           marker->parent_runtime_uid==0)continue;
        bool linked_body=false;
        for(std::size_t j=0;j<world.placement_count();++j){
            const auto* body=world.placement(j);
            if(!body || body->runtime_uid!=marker->parent_runtime_uid)
                continue;
            linked_body=minidayz::stage1::source_family_contains(1041,body->object);
            assert(linked_body);
            assert(body->source_variable_1==marker->runtime_uid);
            break;
        }
        assert(linked_body && minidayz::items::original_car_pool(marker->source_frame));
        assert(minidayz::items::original_car_trunk_has_source_body(world,*marker));
        ++generated_car_trunks;
    }
    std::fprintf(stderr,
        "ROOT GENERATION CARS seed=4d444159 searchable_trunks=%zu\n",
        generated_car_trunks);
    assert(generated_car_trunks>0);

}

void test_original_tree_block_variants_and_solid_companions() {
    // Directly verified root create_map_element event branches 7..10.
    // Object210 has four *different frames*, each with an original linked
    // Solid348 at its Imagepoint1 and frames 17/21/22/23. All were missing.
    minidayz::WorldSystem world;
    assert(world.initialize(0x210348u));
    for(std::uint8_t variant=7;variant<=10;++variant){
        const auto* spec=minidayz::world_source::source_tree_block_variant(variant);
        assert(spec && spec->tree_frame==static_cast<std::uint32_t>(variant-7u));
        const auto first=world.placement_count();
        assert(world.expand_map_element(1,variant,0,
            {8000.0f+variant*350.0f,8000.0f},0.0)==1);
        assert(world.placement_count()==first+2);
        const auto* tree=world.placement(first);
        const auto* solid=world.placement(first+1);
        assert(tree&&solid&&tree->object==210&&solid->object==348);
        assert(tree->source_frame==spec->tree_frame);
        assert(tree->width==spec->width&&tree->height==spec->height);
        assert(tree->origin_x==spec->origin_x);
        assert(tree->origin_y==spec->origin_y);
        assert(solid->source_frame==spec->solid_frame);
        assert(solid->width==spec->solid_width);
        assert(solid->height==spec->solid_height);
        assert(solid->origin_x==spec->solid_origin_x);
        assert(solid->origin_y==spec->solid_origin_y);
        assert(tree->linked_runtime_uid==solid->runtime_uid);
        assert(solid->linked_runtime_uid==tree->runtime_uid);
        assert(tree->source_variable_1==solid->runtime_uid);
        assert(solid->source_variable_0==tree->runtime_uid);
        // Frame21's actual mask is below its hotspot. Do not assume
        // every source collision polygon contains its own origin.
        const auto* mask=minidayz::stage1::source_building_collision_polygon(
            solid->object,solid->source_animation,solid->source_frame);
        assert(mask && mask->points>=3);
        const auto q=minidayz::world_source::source_house_mask_vertex(*solid,0);
        assert(minidayz::terrain::source_tree_block_solid_contains(*tree,q.x,q.y));
        assert(minidayz::terrain::source_tree_block_solid_contains(
            *tree,solid->x,solid->y)==
            minidayz::terrain::source_world_solid_mask_contains(*solid,solid->x,solid->y));
        assert(!minidayz::terrain::source_tree_block_solid_contains(
            *tree,solid->x+1500.0f,solid->y+1500.0f));
    }
    minidayz::terrain::SourceObstacleIndex index;
    index.rebuild(world);
    for(std::size_t i=0;i<world.placement_count();i+=2){
        const auto* solid=world.placement(i+1);
        assert(solid);
        const auto& candidate=index.candidates(solid->x,solid->y);
        assert(std::find(candidate.begin(),candidate.end(),
            static_cast<std::uint32_t>(i))!=candidate.end());
        assert(std::find(candidate.begin(),candidate.end(),
            static_cast<std::uint32_t>(i+1))!=candidate.end());
    }
}

void test_source_event_tree_hit_health_and_atomic_fall() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    const auto* rules=tree_hit_rule(207);
    assert(rules && rules->hit_event_uid==8754965976446553ULL);
    assert(rules->fall_event_uid==2738244684146745ULL);
    for (const auto& rule:kTreeHitRules) {
        const auto* fall=tree_fall_rule(rule.object_id);
        assert(fall && fall->event_uid==rule.fall_event_uid);
    }

    WorldSystem world;
    assert(world.generate_from_source(0x4d444159u,"Map",0));
    std::size_t tree=world.placement_count(),other=tree;
    for (std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if (p && p->object==207 && p->source_frame==0) {
            if (tree==world.placement_count())tree=i;
            else {other=i;break;}
        }
    }
    assert(tree<world.placement_count() && other<world.placement_count());
    const auto* p=world.placement(tree);
    const std::uint64_t id=p->runtime_uid;
    const auto other_id=world.placement(other)->runtime_uid;
    // Every procedural tree creates the original separate frame-10
    // obst_base hitbox at its frame-0 Imagepoint 1. The linked IDs are
    // essential to the original object-345 projectile collision event.
    const std::uint64_t hitbox_uid=p->linked_runtime_uid;
    assert(hitbox_uid!=0);
    std::size_t hitbox_index=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* q=world.placement(i);
        if(q && q->runtime_uid==hitbox_uid){hitbox_index=i;break;}
    }
    assert(hitbox_index<world.placement_count());
    const auto* hitbox=world.placement(hitbox_index);
    assert(hitbox->object==345 && hitbox->source_frame==10);
    assert(hitbox->linked_runtime_uid==id);
    // Regression: actual tree frame dimensions and authored Imagepoint1
    // agree with the separate visible 345 standing trunk collider.
    assert(minidayz::terrain::source_standing_tree_trunk_contains(
        *p,hitbox->x,hitbox->y));
    assert(!minidayz::terrain::source_standing_tree_trunk_contains(
        *p,hitbox->x+120.0f,hitbox->y+120.0f));
    assert(minidayz::terrain::source_tree_occludes_player(
        *p,p->x,p->y-0.25f*p->height));
    assert(!minidayz::terrain::source_tree_occludes_player(
        *p,p->x+2000.0f,p->y));
    // A translucent foreground tree must NOT leave a nearer depth in the
    // GPU buffer and hide the subsequently drawn source layer32 player.
    assert(minidayz::terrain::source_tree_render_depth(
        0.6875f,0.55f,true)<0.55f);
    assert(minidayz::terrain::source_tree_render_depth(
        0.6875f,0.55f,false)==0.6875f);
    minidayz::terrain::SourceObstacleIndex tree_index{};
    tree_index.rebuild(world);
    // Verify EVERY procedurally placed original tree species, not just
    // the first Sprite207. The original create_map_element attaches
    // Sprite345 frame10 at the tree's authored Imagepoint1. If the
    // broadphase misses either linked object, players walk through trees.
    std::unordered_map<std::uint64_t,std::size_t> tree_uid_to_index;
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* placed=world.placement(i);
        if(placed && placed->runtime_uid)
            tree_uid_to_index.emplace(placed->runtime_uid,i);
    }
    std::size_t verified_original_trunks=0;
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* visible=world.placement(i);
        if(!visible || visible->layer!=43 || visible->source_uid==0 ||
           visible->source_frame!=0 ||
           !source_tree_impact_point(visible->object))continue;
        assert(visible->linked_runtime_uid!=0);
        const auto companion=tree_uid_to_index.find(visible->linked_runtime_uid);
        assert(companion!=tree_uid_to_index.end());
        const auto* trunk=world.placement(companion->second);
        assert(trunk && trunk->object==345 && trunk->source_frame==10);
        assert(trunk->linked_runtime_uid==visible->runtime_uid);
        float expected_x{},expected_y{};
        assert(minidayz::terrain::source_standing_tree_trunk_position(
            *visible,expected_x,expected_y));
        assert(std::fabs(trunk->x-expected_x)<0.05f);
        assert(std::fabs(trunk->y-expected_y)<0.05f);
        assert(minidayz::terrain::source_standing_tree_trunk_contains(
            *visible,trunk->x,trunk->y));
        const auto& colliders=tree_index.candidates(trunk->x,trunk->y);
        assert(std::find(colliders.begin(),colliders.end(),
            static_cast<std::uint32_t>(i))!=colliders.end());
        assert(std::find(colliders.begin(),colliders.end(),
            static_cast<std::uint32_t>(companion->second))!=colliders.end());
        ++verified_original_trunks;
    }
    assert(verified_original_trunks>=100);
    const auto& trunk_candidates=tree_index.candidates(hitbox->x,hitbox->y);
    assert(std::find(trunk_candidates.begin(),trunk_candidates.end(),
         static_cast<std::uint32_t>(tree))!=trunk_candidates.end());
    const auto* original_point=source_tree_impact_point(207);
    assert(original_point);
    const float a=p->angle*0.01745329251994329577f;
    const float dx=(original_point->point_x-original_point->origin_x)*p->width;
    const float dy=(original_point->point_y-original_point->origin_y)*p->height;
    assert(std::fabs(hitbox->x-(p->x+dx*std::cos(a)-dy*std::sin(a)))<0.05f);
    assert(std::fabs(hitbox->y-(p->y+dx*std::sin(a)+dy*std::cos(a)))<0.05f);
    std::array<WorldPoint,4> drops{};
    for(std::size_t i=0;i<drops.size();++i)
        drops[i]={p->x+static_cast<float>(i),p->y+40.0f};

    items::GroundLootSystem loot;
    float hp=2.0f; // TEST FIXTURE; NOT asserted as original initial HP.
    assert(apply_verified_tree_hit(world,id,999,hp,drops,loot)
           ==TreeHitOutcome::Rejected);
    assert(hp==2.0f && loot.count()==0);
    assert(apply_verified_tree_hit(world,other_id,8754965976446553ULL,
                                  hp,drops,loot)==TreeHitOutcome::Damaged);
    assert(hp==1.0f && world.placement(other)->source_frame==0);
    // Use the original affected tree's independent test health now.
    float tree_hp=2.0f;
    assert(apply_verified_tree_hit(world,id,8754965976446553ULL,
                                  tree_hp,drops,loot)==TreeHitOutcome::Damaged);
    assert(tree_hp==1.0f && loot.count()==0);
    assert(world.placement(tree)->source_frame==0);
    assert(apply_verified_tree_hit(world,id,8754965976446553ULL,
                                  tree_hp,drops,loot)==TreeHitOutcome::Fallen);
    assert(tree_hp==0.0f && loot.count()==3);
    assert(world.placement(tree)->source_frame==1);
    assert(world.placement(hitbox_index)->source_frame==8);
    assert(loot.entry(0)->item.item_id==38);
    assert(loot.entry(1)->item.item_id==38);
    assert(loot.entry(2)->item.item_id==50);
    assert(apply_verified_tree_hit(world,id,8754965976446553ULL,
                                  tree_hp,drops,loot)==TreeHitOutcome::Rejected);
    assert(loot.count()==3 && tree_hp==0.0f);
    assert(world.placement(other)->source_frame==0);
    assert(!world.capture_sprite_states().empty());
    // Bad source RNG positions cannot silently consume HP.
    WorldSystem second;
    assert(second.generate_from_source(0x4d444159u,"Map",0));
    const auto uid=second.placement(tree)->runtime_uid;
    drops[0].x=std::numeric_limits<float>::infinity();
    float final_hp=1.0f;
    items::GroundLootSystem empty;
    assert(apply_verified_tree_hit(second,uid,8754965976446553ULL,
                                  final_hp,drops,empty)==TreeHitOutcome::Rejected);
    assert(final_hp==1.0f && empty.count()==0);
    assert(second.placement(tree)->source_frame==0);
}

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
    std::size_t generated=0,trees=0,source_sized_trees=0;
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
            // Every procedurally created tree's first Construct frame
            // provides the default size and hotspot. Render, rotation
            // and collision cannot assume a zero-size, zero-origin sprite.
            if(a->layer==43 && a->source_uid!=0 && a->object==207) {
                const auto* def=minidayz::stage1::find_object(a->object);
                assert(def && def->width>0.0f && def->height>0.0f);
                assert(a->width==def->width && a->height==def->height);
                assert(a->origin_x==def->origin_x);
                assert(a->origin_y==def->origin_y);
                ++source_sized_trees;
            }
        } else {
            // Original static objects keep their authored instance UID.
            assert(a->runtime_uid==a->source_uid);
        }
    }
    assert(generated>1000);
    // Every original source tree species has valid frame0 geometry and
    // is accompanied by an invisible 345 trunk, not an invented full
    // rectangle over foliage. Catch the broken 0x0 Imagepoint fallback.
    std::size_t source_trees_verified=0;
    for(std::size_t i=0;i<first.placement_count();++i) {
        const auto* tree=first.placement(i);
        if(!tree || tree->source_uid==0)continue;
        const auto* frame=minidayz::world_source::source_tree_impact_point(tree->object);
        if(!frame)continue;
        assert(tree->width==frame->width && tree->height==frame->height);
        assert(tree->origin_x==frame->origin_x &&
               tree->origin_y==frame->origin_y);
        assert(tree->linked_runtime_uid!=0);
        ++source_trees_verified;
    }
    assert(source_trees_verified>50);
    assert(trees>1000);
    assert(source_sized_trees>0);
    assert(!first.placement_by_id(0));
    assert(!first.placement_by_instance_id(0));
    assert(!first.placement_by_instance_id(~std::uint64_t{0}));
}

void test_source_visual_state_snapshot_roundtrip() {
    constexpr auto seed=0x4d445a33u;
    minidayz::WorldSystem world, replay;
    assert(world.generate_from_source(seed,"Map",0));
    assert(replay.generate_from_source(seed,"Map",0));
    assert(world.capture_sprite_states().empty());
    std::size_t first=world.placement_count();
    std::size_t second=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(!p || p->object!=207 || p->layer!=43) continue;
        if(first==world.placement_count()) first=i;
        else {second=i;break;}
    }
    assert(first<world.placement_count() && second<world.placement_count());
    const auto uid=world.placement(first)->runtime_uid;
    const auto other=*world.placement(second);
    assert(world.set_sprite_state_by_runtime_uid(uid,0,1,1.0f));
    assert(world.placement(first)->source_frame==1);
    assert(world.placement(second)->source_frame==other.source_frame);
    const auto changed=world.capture_sprite_states();
    assert(changed.size()==1 && changed[0].runtime_uid==uid);
    assert(replay.restore_sprite_states(changed));
    assert(replay.placement(first)->source_frame==1);
    assert(replay.placement(second)->source_frame==other.source_frame);
    auto bad=changed;
    bad.push_back(bad[0]);
    assert(!replay.restore_sprite_states(bad));
    assert(replay.placement(first)->source_frame==1);
    bad=changed;
    bad[0].frame=999999;
    assert(!replay.restore_sprite_states(bad));
    assert(replay.placement(first)->source_frame==1);
    bad=changed;
    bad[0].object=209;
    assert(!replay.restore_sprite_states(bad));
    assert(replay.placement(first)->source_frame==1);
    bad=changed;
    bad[0].runtime_uid=~std::uint64_t{0};
    assert(!replay.restore_sprite_states(bad));
    assert(replay.placement(first)->source_frame==1);
    assert(replay.restore_sprite_states({}));
    assert(replay.capture_sprite_states().empty());
    assert(replay.placement(first)->source_frame==world.placement(second)->source_frame);
}

void test_original_world_visual_file_roundtrip() {
    constexpr std::uint32_t seed=0x4d445a33u;
    constexpr const char* path="minidayz_world_visual_test.bin";
    constexpr const char* temp="minidayz_world_visual_test.tmp";
    constexpr const char* corrupt="minidayz_world_visual_corrupt.bin";
    std::remove(path);
    std::remove(temp);
    std::remove(corrupt);
    minidayz::WorldSystem first, replay, wrong;
    assert(first.generate_from_source(seed,"Map",0));
    assert(replay.generate_from_source(seed,"Map",0));
    assert(wrong.generate_from_source(seed+1,"Map",0));
    std::size_t tree=first.placement_count();
    for(std::size_t i=0;i<first.placement_count();++i) {
        const auto* p=first.placement(i);
        if(p && p->layer==43 && p->object==207) {tree=i;break;}
    }
    assert(tree<first.placement_count());
    const auto uid=first.placement(tree)->runtime_uid;
    assert(first.set_sprite_state_by_runtime_uid(uid,0,1,1.0f));
    // Source roof event 9848716569903957 moves one family-1049 member
    // to Buildings_bg. The journal must restore both its opacity and
    // its actual draw layer, not just the Sprite frame.
    std::size_t roof=first.placement_count();
    for(std::size_t i=0;i<first.placement_count();++i)
        if(minidayz::stage1::source_family_contains(1049,first.placement(i)->object)){
            roof=i;break;
        }
    assert(roof<first.placement_count());
    const auto source_layer=first.placement(roof)->layer;
    const auto* map=minidayz::stage1::find_layout("Map");
    assert(map);
    const auto map_id=static_cast<std::uint16_t>(map-minidayz::stage1::kLayoutTable);
    std::uint16_t bg=0xffff;
    for(const auto& l:minidayz::stage1::kLayerTable)
        if(l.layout==map_id && std::strcmp(l.name,"Buildings_bg")==0){
            bg=l.index;break;
        }
    assert(bg!=0xffff);
    assert(first.set_placement_layer(roof,bg));
    assert(first.set_placement_sprite_state(
        roof,first.placement(roof)->source_animation,
        first.placement(roof)->source_frame,0.25f));
    assert(!first.capture_sprite_states().empty());
    using minidayz::persistence::write_world_visual_state;
    using minidayz::persistence::read_world_visual_state;
    assert(!write_world_visual_state(first,0,path,path));
    assert(write_world_visual_state(first,0,path,temp));
    assert(!read_world_visual_state(wrong,0,path));
    assert(!read_world_visual_state(replay,1,path));
    assert(replay.capture_sprite_states().empty());
    assert(read_world_visual_state(replay,0,path));
    assert(replay.placement(tree)->source_frame==1);
    assert(replay.placement(roof)->layer==bg);
    assert(replay.placement(roof)->opacity==0.25f);
    assert(!replay.capture_sprite_states().empty());
    auto* in=std::fopen(path,"rb");
    assert(in);
    auto* out=std::fopen(corrupt,"wb");
    assert(out);
    unsigned char bytes[256]{};
    const auto n=std::fread(bytes,1,sizeof(bytes),in);
    assert(n>25);
    bytes[22]^=0x10; // checksum must reject tampering
    assert(std::fwrite(bytes,1,n,out)==n);
    assert(std::fclose(in)==0);
    assert(std::fclose(out)==0);
    assert(!read_world_visual_state(replay,0,corrupt));
    assert(replay.placement(tree)->source_frame==1);
    assert(replay.restore_sprite_states({}));
    assert(replay.capture_sprite_states().empty());
    assert(replay.placement(roof)->layer==source_layer);
    assert(read_world_visual_state(replay,0,path));
    assert(replay.placement(tree)->source_frame==1);
    assert(replay.placement(roof)->layer==bg);
    std::remove(path);
    std::remove(temp);
    std::remove(corrupt);
}

void test_source_event_tree_fall_and_door_transition() {
    constexpr auto seed=0x4d444159u;
    minidayz::WorldSystem world;
    assert(world.generate_from_source(seed,"Map",0));
    std::size_t tree=world.placement_count();
    std::size_t door=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(!p)continue;
        if(tree==world.placement_count() && p->object==207 &&
           p->source_animation==0 && p->source_frame==0)tree=i;
        if(door==world.placement_count() && p->object==349 &&
           p->source_frame==0)door=i;
        if(tree<world.placement_count() && door<world.placement_count())break;
    }
    assert(tree<world.placement_count());
    const auto* p=world.placement(tree);
    const auto uid=p->runtime_uid;
    std::array<minidayz::WorldPoint,4> positions{};
    for(std::size_t i=0;i<positions.size();++i)
        positions[i]={p->x+static_cast<float>(i*3),p->y+44.0f};
    minidayz::items::GroundLootSystem loot;
    using minidayz::world_source::apply_verified_tree_fall;
    constexpr auto fall=2738244684146745ULL;
    assert(!apply_verified_tree_fall(world,uid,123,positions,loot));
    assert(loot.count()==0);
    assert(apply_verified_tree_fall(world,uid,fall,positions,loot));
    assert(world.placement(tree)->source_frame==1);
    assert(loot.count()==3);
    assert(loot.entry(0)->item.item_id==38);
    assert(loot.entry(1)->item.item_id==38);
    assert(loot.entry(2)->item.item_id==50);
    assert(!apply_verified_tree_fall(world,uid,fall,positions,loot));
    assert(loot.count()==3);
    // No unrelated tree is permitted to change from this one event.
    for(std::size_t i=tree+1;i<world.placement_count();++i) {
        const auto* other=world.placement(i);
        if(other && other->object==207) {
            assert(other->source_frame==0);break;
        }
    }
    if(door<world.placement_count()) {
        const auto did=world.placement(door)->runtime_uid;
        using minidayz::world_source::apply_verified_door_frame;
        assert(!apply_verified_door_frame(world,did,fall));
        assert(apply_verified_door_frame(world,did,6061166023614368ULL));
        assert(world.placement(door)->source_frame==1);
        assert(!apply_verified_door_frame(world,did,6061166023614368ULL));
        assert(apply_verified_door_frame(world,did,243854400993751ULL));
        assert(world.placement(door)->source_frame==0);
        // Original alternate prompt-triggered door group opens at
        // UID 7258332154939593 and closes at 353730214514351.
        // Their other action (destroy prompt object 506) is not a
        // reason to reject the exact original door frame transition.
        assert(apply_verified_door_frame(world,did,7258332154939593ULL));
        assert(world.placement(door)->source_frame==1);
        assert(!apply_verified_door_frame(world,did,7258332154939593ULL));
        assert(apply_verified_door_frame(world,did,353730214514351ULL));
        assert(world.placement(door)->source_frame==0);
        const auto* door_state=world.placement(door);
        if(door_state->linked_runtime_uid!=0) {
            const auto instance=door_state->linked_runtime_uid&
                ~(std::uint64_t{1}<<63);
            const auto* solid=world.placement_by_instance_id(instance);
            assert(solid && solid->object==348 &&
                   solid->opacity==1.0f);
        }
    }
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

    // Generated buildings and foliage now inherit their actual original
    // Construct frame size and hotspot. The broadphase must still
    // retain sprites crossing a 512px visibility-bin boundary; initially
    // zero-size editor Create records are no longer a requirement.
    std::size_t verified_sprite_crossings=0;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* p=world.placement(i);
        if(!p || p->layer==32 || p->layer>=44 || p->opacity<=0.0f)continue;
        const auto* obj=minidayz::stage1::find_object(p->object);
        if(!obj || obj->type!=18)continue;
        const float source_extent=std::hypot(
            std::max(p->width,obj->width),std::max(p->height,obj->height));
        if(source_extent<70.0f)continue;
        const float boundary=(std::floor(p->x/512.0f)+1.0f)*512.0f;
        const float dx=boundary-p->x;
        if(dx<=0.0f || dx>=source_extent ||
           boundary+2.0f>=world.bounds().width)continue;
        std::vector<std::uint32_t> cross_bin{};
        visibility.query(boundary+1.0f,p->y,1.0f,1.0f,cross_bin);
        assert(std::binary_search(cross_bin.begin(),cross_bin.end(),
                    static_cast<std::uint32_t>(i)));
        ++verified_sprite_crossings;
        if(verified_sprite_crossings>=3)break;
    }
    assert(verified_sprite_crossings>0);

    // Likewise, player corner-collision queries must never scan every
    // building in the original 36,850x20,000 world.
    minidayz::terrain::SourceObstacleIndex collision{};
    collision.rebuild(world);
    assert(collision.candidates(600.0f,5000.0f).size()<
           world.placement_count());
    // Root create_map_element spawns the original invisible obst_base 345
    // onto zed_resp (not Obstacle_enviroment), linked to a visible tree.
    // Both broadphase and player narrowphase must treat ONLY these linked
    // Sprite345 hitboxes as solid; ordinary zed_resp markers aren't solid.
    const auto* map_layout=minidayz::stage1::find_layout("Map");
    assert(map_layout);
    const auto layout_id=static_cast<std::uint16_t>(
        map_layout-minidayz::stage1::kLayoutTable);
    std::size_t original_obstacle_bases=0;
    std::size_t linked_tree_bases=0;
    for (std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(!p || p->object!=345 || p->width<=0 || p->height<=0 ||
           !world.bounds().contains({p->x,p->y}))continue;
        const auto* layer=minidayz::stage1::find_layer(layout_id,p->layer);
        if(!layer)continue;
        const bool tree=minidayz::terrain::source_linked_tree_collision_hitbox(
            p->object,p->linked_runtime_uid,layer->name);
        if(!tree && std::strcmp(layer->name,"Obstacle_enviroment")!=0)
            continue;
        const auto& nearby=collision.candidates(p->x,p->y);
        assert(std::find(nearby.begin(),nearby.end(),
                   static_cast<std::uint32_t>(i))!=nearby.end());
        if(tree && p->source_frame==
           minidayz::world_source::kOriginalTreeStandingHitboxFrame)
            ++linked_tree_bases;
        ++original_obstacle_bases;
    }
    std::printf("ORIGINAL COLLIDER BASES indexed=%zu linked_tree=%zu\\n",
                original_obstacle_bases,linked_tree_bases);
    assert(linked_tree_bases>50);


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
    // The Construct source event/action UID can create MORE THAN ONE
    // object216 at different map sites. Those are different instances,
    // even if their original creation action UID is identical. Regression
    // against NPC identity/random-skin/offset accidentally collapsing.
    std::unordered_set<std::uint64_t> seen_spawn_instances;
    std::unordered_map<std::uint64_t,std::uint32_t> seeded_action_instances;
    std::size_t npc_index=0;
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* spawn=world.placement(i);
        if(!spawn || spawn->object!=216)continue;
        const auto* actor=npcs.actor(npc_index++);
        assert(actor && spawn->runtime_uid!=0);
        assert(seen_spawn_instances.insert(spawn->runtime_uid).second);
        if(spawn->source_uid==0)continue;
        const auto found=seeded_action_instances.find(spawn->source_uid);
        if(found==seeded_action_instances.end())
            seeded_action_instances.emplace(spawn->source_uid,actor->random_state);
        else
            assert(actor->random_state!=found->second &&
                "Two NPCs from one original source Spawn action reused one RNG seed");
    }
    assert(npc_index==npcs.count());
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

void test_original_perk_dao_world_scatter_branch() {
    // Root UIDs 606281189890536 / 982873373809928 /
    // 990933237254084 are OR blocks: perk_dao==5 OR CurrentLevel==4.
    // Their immediately following Else nodes select ordinary art.
    std::size_t special_count=0,else_count=0;
    for(const auto& rule:minidayz::stage1::kTerrainScatterRules){
        if(rule.perk_gate==1){
            ++special_count;
            assert(rule.level_mask==0x3fu);
            assert(rule.repeat==200);
        } else if(rule.perk_gate==2)++else_count;
        else assert(rule.perk_gate==0);
    }
    assert(special_count>=4 && else_count>=4);
    constexpr std::uint32_t seed=0x45abc123u;
    minidayz::WorldSystem ordinary,perked;
    assert(ordinary.generate_from_source(seed,"Map",4,0));
    assert(perked.generate_from_source(seed,"Map",4,5));
    assert(ordinary.generation_perk_dao()==0);
    assert(perked.generation_perk_dao()==5);
    minidayz::WorldTravel travel;
    assert(travel.begin(seed,4,5));
    assert(travel.destination()!=nullptr);
    assert(travel.destination()->generation_perk_dao()==5);
    travel.cancel();
    assert(ordinary.map_marker_count()==perked.map_marker_count());
    for(std::size_t i=0;i<ordinary.map_marker_count();++i){
        const auto* a=ordinary.map_marker(i);
        const auto* b=perked.map_marker(i);
        assert(a&&b&&a->marker_type==b->marker_type);
    }
    bool changed=ordinary.source_scatter_patches().size()!=
                 perked.source_scatter_patches().size();
    const auto& left=ordinary.source_scatter_patches();
    const auto& right=perked.source_scatter_patches();
    for(std::size_t i=0;i<left.size()&&i<right.size();++i){
        if(left[i].marker_index!=right[i].marker_index ||
           left[i].tiles.size()!=right[i].tiles.size()){
            changed=true;continue;
        }
        for(std::size_t k=0;k<left[i].tiles.size();++k)
            if(left[i].tiles[k].local_index!=right[i].tiles[k].local_index ||
               left[i].tiles[k].original_frame!=right[i].tiles[k].original_frame)
                changed=true;
    }
    // Both are already level4: changing the perk cannot change these
    // scatter guards. Earlier approximations treated the OR as AND.
    assert(!changed);
    minidayz::WorldGrid grid;
    assert(grid.resize(16,16));
    std::mt19937 a_rng(seed),b_rng(seed),c_rng(seed);
    minidayz::world_source::SourceLocationTerrain plain(grid,a_rng,4,2,2,3,0);
    minidayz::world_source::SourceLocationTerrain perk(grid,b_rng,4,2,2,3,5);
    minidayz::world_source::SourceLocationTerrain snow(grid,c_rng,4,2,2,4,0);
    assert(plain.execute() && perk.execute() && snow.execute());
    assert(plain.scatter!=perk.scatter);
    assert(perk.scatter==snow.scatter);
}

void test_original_special_terrain_contexts() {
    using namespace minidayz::world_source;
    using E=SourceSpecialTerrainEvent;
    static_assert(kSourceLocationTerrainPaintCount+kSourceSpecialTerrainPaintCount==638);
    static_assert(kSourceTotalTerrainPaintCount==638);
    std::mt19937 a(8271),b(8271),c(8271),d(8271);
    minidayz::WorldSpecialTerrain load{},reload{},tutorial{},menu{};
    assert(build_special_terrain(E::BridgeLoad,a,load));
    assert(build_special_terrain(E::BridgeReload,b,reload));
    assert(load.writes==250 && reload.writes==250 && load.cells==reload.cells && a==b);
    assert(load.source_action_uid==895209268453600ULL);
    assert(reload.source_action_uid==458228549101251ULL);
    assert(load.x==148&&load.y==18391&&reload.y==18351);
    for(int y=0;y<load.rows;++y)for(int x=231;x<load.columns;++x)
        assert(load.cells[y*load.columns+x]==0xffffffffu);
    assert(build_special_terrain(E::TutorialStart,c,tutorial));
    assert(tutorial.writes==1250&&tutorial.source_action_uid==506106248958134ULL);
    assert(tutorial.columns==226&&tutorial.rows==97&&tutorial.x==693&&tutorial.y==533);
    for(int y=26;y<tutorial.rows;++y)for(int x=0;x<tutorial.columns;++x){
        const auto i=y*tutorial.columns+x;
        assert(tutorial.cells[i]==kTutorialStartAuthored[i]);
    }
    assert(build_special_terrain(E::MenuStart,d,menu));
    assert(menu.writes==400&&menu.source_action_uid==2084078328497493ULL);
    assert(menu.columns==91&&menu.rows==50&&menu.x==-9&&menu.y==-6);
    for(int y=0;y<menu.rows;++y)for(int x=83;x<menu.columns;++x){
        const auto i=y*menu.columns+x;
        assert(menu.cells[i]==kMenuStartAuthored[i]);
    }
    minidayz::WorldSystem world;
    assert(world.initialize(8271));
    assert(world.load_source_layout("Map"));
    assert(world.source_special_terrain().empty());
    assert(!world.run_source_bridge_terrain(false,true));
    assert(world.run_source_bridge_terrain(true,false));
    assert(world.source_special_terrain().source_action_uid==load.source_action_uid);
    assert(world.source_bridge_active());
    assert(world.source_bridge_solids().size()==4);
    const auto initial_solids=world.source_bridge_solids();
    assert(initial_solids[0].source_uid==405637541386025ULL);
    assert(initial_solids[0].source_frame==26);
    assert(initial_solids[0].x==48 && initial_solids[0].y==18356);
    assert(initial_solids[0].width==7200 && initial_solids[0].height==50);
    assert(initial_solids[1].height==100 && initial_solids[2].height==350);
    assert(initial_solids[3].x==7148);
    assert(world.run_source_bridge_terrain(true,true));
    assert(world.source_special_terrain().source_action_uid==reload.source_action_uid);
    assert(world.source_bridge_solids().size()==4);
    assert(world.source_bridge_solids()[0].source_uid==272428373652722ULL);
    for(std::size_t i=0;i<4;++i){
        const auto& p=world.source_bridge_solids()[i];
        assert(p.x==initial_solids[i].x && p.y==initial_solids[i].y);
        assert(p.width==initial_solids[i].width && p.height==initial_solids[i].height);
    }
    assert(world.load_source_layout("Tutorial"));
    assert(!world.source_bridge_active());
    assert(world.source_special_terrain().writes==1250);
    assert(!world.run_source_bridge_terrain(true,true));
    assert(world.load_source_layout("Menu"));
    assert(world.source_special_terrain().writes==400);
    assert(world.load_source_layout("Map")&&world.source_special_terrain().empty());
}

void test_compiled_original_location_terrain_program() {
    using minidayz::world_source::SourceLocationTerrain;
    static_assert(minidayz::world_source::kSourceLocationTerrainPaintCount==634);
    minidayz::WorldGrid grid;
    assert(grid.resize(16,33));
    std::mt19937 rng(0x4d445a33u);
    // Exercise every distinct source location marker at each level and
    // both halves, including the original perk-dao theme selectors.
    constexpr int markers[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
        21,22,23,24,25,26,27,28,29,30,31,32,40,41,42,43,44,45,46,47,
        48,49,50,51,52,60,61,62,63,64,65,66,67,70,99,100,101,102,
        103,104,105,106,107,108,109,110,998,999};
    for(int level=0;level<6;++level)
        for(int row:{2,17,18})
            for(int marker:markers){
                SourceLocationTerrain p(grid,rng,4,row,marker,level,
                                        level==5?7:0);
                assert(p.execute());
                assert(p.ground_object!=0);
                if(row>=17)assert(p.ground_object==1138);
                for(auto frame:p.ground)assert(frame>=0&&frame<25);
                if(marker==3){
                    assert(p.water_created);
                    assert(p.water[8*17+8]==4);
                }
            }
    // Original family1066 paints road code12 across local y8..9;
    // original source origin is -500, not the old -510 road model.
    SourceLocationTerrain road(grid,rng,4,2,12,0,0);
    assert(road.execute() && road.ground_object==482);
    assert(road.ground_x==-500 && road.ground_y==-500);
    assert(road.ground[8*17+8]==15&&road.ground[9*17+8]==15);
    assert(road.ground[7*17+8]==11&&road.ground[10*17+8]==1);
    // Original village event 9929737684894666 accepts only Array518
    // road values 12, 13, 14 for its negative-column connection.
    assert(grid.set(3,2,9));
    SourceLocationTerrain unconnected(grid,rng,4,2,9,0,0);
    assert(unconnected.execute());
    assert(unconnected.ground[8]!=15);
    for(int neighbor:{12,13,14}){
        assert(grid.set(3,2,neighbor));
        SourceLocationTerrain village(grid,rng,4,2,9,0,0);
        assert(village.execute());
        assert(village.ground[8]==15);
    }
}

void test_source_scatter_reproduction_and_layout_replacement() {
    // Original ground scatter must follow the same world seed, regardless
    // of camera movement or whether the renderer exists.
    const std::uint32_t seed=0x345aabe2u;
    minidayz::WorldSystem a,b;
    assert(a.generate_from_source(seed,"Map",0));
    assert(b.generate_from_source(seed,"Map",0));
    const auto& lhs=a.source_scatter_patches();
    const auto& rhs=b.source_scatter_patches();
    assert(!lhs.empty() && lhs.size()==rhs.size());
    for(std::size_t p=0;p<lhs.size();++p){
        assert(lhs[p].marker_index==rhs[p].marker_index);
        assert(lhs[p].tiles.size()==rhs[p].tiles.size());
        for(std::size_t k=0;k<lhs[p].tiles.size();++k){
            assert(lhs[p].tiles[k].local_index==rhs[p].tiles[k].local_index);
            assert(lhs[p].tiles[k].original_frame==
                   rhs[p].tiles[k].original_frame);
        }
    }
    const auto& ground_a=a.source_ground_patches();
    const auto& ground_b=b.source_ground_patches();
    assert(ground_a.size()==a.map_marker_count() && ground_a.size()==ground_b.size());
    std::size_t painted=0,edges=0,aligned_roads=0;
    for(std::size_t p=0;p<ground_a.size();++p){
        const auto& ga=ground_a[p];
        const auto& gb=ground_b[p];
        assert(ga.marker_index==p && gb.marker_index==p);
        assert(ga.source_object==gb.source_object&&ga.cells==gb.cells);
        const auto* marker=a.map_marker(p);
        assert(marker);
        if(ga.source_object && (marker->marker_type==12 || marker->marker_type==13)){
            // Root startup action 8480552636662138 shifts the marker by 500
            // before create_map_location creates ground at shifted X/Y-500.
            // The original road tile origin equals the pre-shift marker.
            assert(ga.offset_x==0 && ga.offset_y==0);
            const auto origin=minidayz::world_source::mapgen_hotspot(marker->cell_x,marker->cell_y);
            for(int ty=0;ty<17;++ty)for(int tx=0;tx<17;++tx){
                const auto frame=ga.cells[ty*17+tx];
                assert(a.source_ground_frame_at(origin.x+tx*60.0f+30.0f,
                                               origin.y+ty*60.0f+30.0f)==frame);
                if(frame==15)++aligned_roads;
            }
        }
        for(auto frame:ga.cells){
            if(frame==15)++painted;
            if(frame>=0&&frame<=12&&frame!=6)++edges;
        }
    }
    assert(painted>0&&edges>0&&aligned_roads>0);
    const auto& pond_a=a.source_pond_patches();
    const auto& pond_b=b.source_pond_patches();
    assert(pond_a.size()==pond_b.size());
    for(std::size_t i=0;i<pond_a.size();++i){
        assert(pond_a[i].marker_index==pond_b[i].marker_index);
        const auto* marker=a.map_marker(pond_a[i].marker_index);
        assert(marker && marker->marker_type==3);
        assert(pond_a[i].tiles.size()==pond_b[i].tiles.size());
        const auto center=minidayz::world_source::mapgen_hotspot(marker->cell_x,marker->cell_y);
        assert(a.source_pond_contains(center.x+510,center.y+510,false));
        assert(a.source_pond_contains(center.x+510,center.y+510,true));
        for(std::size_t j=0;j<pond_a[i].tiles.size();++j){
            assert(pond_a[i].tiles[j].local_index==pond_b[i].tiles[j].local_index);
            assert(pond_a[i].tiles[j].original_frame==pond_b[i].tiles[j].original_frame);
        }
    }
    assert(a.load_source_layout("Map"));
    // A second authored layout must not inherit a previous generated
    // forest, loot markers or a misleading completed-generation phase.
    assert(!a.generation_complete());
    assert(a.source_scatter_patches().empty());
    assert(a.source_ground_patches().empty());
    assert(a.source_pond_patches().empty());
    assert(a.map_marker_count()==0);
    assert(a.source_location_spawn_count()==0);
}

// New Game must not reuse an old, static Map. Confirm that two different
// seeds change the original-generated grid AND populate the display layers
// with original Construct sprites that the 3DS world renderer can draw.
void test_generated_original_exterior_roofs_visible_in_world() {
    // Original city/piano exteriors from the root Construct object pairs
    // must be present as *playable* generated placements. Counting only
    // invisible zed_resp triggers is not evidence that buildings appear.
    constexpr std::uint32_t seeds[]={0x10203040u,123456u};
    for(const auto seed:seeds) {
        minidayz::WorldSystem world;
        assert(world.generate_from_source(seed,"Map",0));
        std::size_t roofs=0;
        std::size_t mapped_roofs=0;
        minidayz::terrain::VisibleSourcePlacementIndex index;
        index.rebuild(world);
        for(std::size_t i=0;i<world.placement_count();++i) {
            const auto* p=world.placement(i);
            if(!p || p->object<321 || p->object>333)continue;
            if(p->object!=321 && p->object!=322 && p->object!=323 &&
               p->object!=324 && p->object!=333)continue;
            if(!(p->opacity>0.0f) || p->layer>=44 ||
               !world.bounds().contains({p->x,p->y}))continue;
            const auto* frame=minidayz::stage1::source_building_frame_geometry(
                p->object,p->source_animation,p->source_frame);
            assert(frame && frame->width>0.0f && frame->height>0.0f);
            ++roofs;
            std::vector<std::uint32_t> visible;
            // Index must keep an original-sized roof accessible where
            // the player camera is actually looking at its hotspot.
            index.query(p->x-200.0f,p->y-120.0f,400.0f,240.0f,visible);
            if(std::find(visible.begin(),visible.end(),
               static_cast<std::uint32_t>(i))!=visible.end())++mapped_roofs;
        }
        std::printf("ORIGINAL GENERATED EXTERIOR ROOFS seed=%u renderable=%zu indexed=%zu\\n",
                    static_cast<unsigned>(seed),roofs,mapped_roofs);
        assert(roofs>0);
        assert(mapped_roofs==roofs);
    }
}

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
        // A source Create/SpawnAnotherObject action must retain its exact
        // Construct ACTION UID on every unique runtime world placement.
        // The old native location adapter discarded these IDs (source_uid=0),
        // so original art/event guards could not trace generated locations.
        std::vector<std::uint64_t> source_create_actions{};
        for(const auto& action:minidayz::stage1::kLocationEventActionSource)
            if((action.object_index==-1 && action.command_id==94) ||
               (action.object_index==514 && action.command_id==73))
                source_create_actions.push_back(action.action_uid);
        assert(!source_create_actions.empty());
        std::sort(source_create_actions.begin(),source_create_actions.end());
        source_create_actions.erase(
            std::unique(source_create_actions.begin(),source_create_actions.end()),
            source_create_actions.end());
        std::size_t source_location_actions_preserved=0;
        for(std::size_t i=0;i<world->placement_count();++i){
            const auto* generated=world->placement(i);
            assert(generated);
            if((generated->runtime_uid & (std::uint64_t{1}<<63)) &&
               std::binary_search(source_create_actions.begin(),
                   source_create_actions.end(),generated->source_uid))
                ++source_location_actions_preserved;
        }
        assert(world->source_location_spawn_count()>0);
        assert(source_location_actions_preserved==
               world->source_location_spawn_count() &&
               "Generated locations lost root source Spawn action identity");
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
        // Source SetTile terrain must be generated with the world, and its
        // original frames must be stable irrespective of camera movement.
        assert(!world->source_scatter_patches().empty());
        std::size_t scatter_cells=0;
        for(const auto& patch:world->source_scatter_patches()){
            const auto* marker=world->map_marker(patch.marker_index);
            assert(marker);
            const auto mask=std::uint8_t{1} << world->generation_level();
            bool source_rule=false;
            for(const auto& rule:minidayz::stage1::kTerrainScatterRules)
                if(rule.marker==marker->marker_type &&
                   (rule.level_mask&mask) && rule.perk_gate!=1)source_rule=true;
            assert(source_rule);
            std::uint16_t previous=0;
            bool first=true;
            for(const auto& tile:patch.tiles){
                assert(tile.local_index<35u*35u);
                if(!first)assert(previous<tile.local_index);
                first=false;previous=tile.local_index;
                assert(tile.original_frame<128);
                ++scatter_cells;
            }
        }
        assert(scatter_cells>0);
        std::size_t drawable_generated_sprites = 0;
        std::size_t generated_loot_markers=0;
        std::size_t original_category15_markers=0;
        for (std::size_t i = 0; i < world->placement_count(); ++i) {
            const auto* candidate=world->placement(i);
            if(candidate && candidate->object==218 &&
               world->bounds().contains({candidate->x,candidate->y})) {
                ++generated_loot_markers;
                if(candidate->object==218 &&
                   candidate->source_loot_category!=0) {
                    assert(candidate->source_variable_0==
                           candidate->source_loot_category);
                }
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

void test_retry_after_failed_world_travel() {
    minidayz::WorldSystem active;
    assert(active.generate_from_source(123456u, "Map", 0));
    const auto placements = active.placement_count();
    const auto markers = active.map_marker_count();
    minidayz::WorldTravel travel;
    constexpr auto destination_seed = 606250596u;
    assert(travel.begin(destination_seed, 255));
    assert(!travel.advance());
    assert(!travel.destination());
    assert(active.seed() == 123456u && active.generation_level() == 0);
    assert(active.placement_count() == placements && active.map_marker_count() == markers);

    assert(travel.begin(destination_seed, 1));
    for (int step = 0; step < 5; ++step) assert(travel.advance());
    assert(travel.destination()->generation_complete());
    assert(travel.destination()->seed() == destination_seed);
    assert(travel.destination()->generation_level() == 1);
    assert(active.seed() == 123456u && active.placement_count() == placements);
}

void test_bunker_and_bridge_placement_reload() {
    using namespace minidayz;
    WorldSystem world;
    assert(world.initialize(42));
    assert(world.load_source_bunker_spawns());
    assert(world.load_source_bridge_elements());
    assert(world.feature_count(WorldFeatureKind::Bunker) == stage1::kBunkerSpawnSourceCount);
    assert(world.feature_count(WorldFeatureKind::Bridge) == stage1::kBridgeElementSourceCount);
    const auto normal = world.feature(0)->position;
    assert(world.load_source_bunker_spawns(true));
    assert(world.feature_count(WorldFeatureKind::Bunker) == stage1::kBunkerSpawnSourceCount);
    const auto alternate = world.feature(stage1::kBridgeElementSourceCount)->position;
    assert(alternate.x == normal.x + 16900.0f);
    assert(alternate.y == normal.y);
    assert(world.load_source_bridge_elements());
    assert(world.feature_count(WorldFeatureKind::Bridge) == stage1::kBridgeElementSourceCount);

    WorldSystem small;
    assert(small.initialize(42, {3600.0f, 20000.0f}));
    assert(small.add_feature({WorldFeatureKind::Bunker, {20.0f, 30.0f}}));
    assert(small.add_feature({WorldFeatureKind::Bridge, {25.0f, 35.0f}}));
    assert(!small.load_source_bunker_spawns());
    assert(!small.load_source_bridge_elements());
    assert(small.feature_count() == 2);
    assert(small.feature(0)->position.x == 20.0f);
    assert(small.feature(1)->position.x == 25.0f);
}

// Road cells must lie between the first pair of anchors on each axis.
void check_generated_road_connections(const minidayz::WorldGrid& grid,
                                      const minidayz::WorldGenerationProfile& profile) {
    const auto anchor = [](std::uint16_t code) {
        return code == 6 || code == 9 || code == 11 || code == 15 || code == 103;
    };
    const auto find_pair = [&](int length, const auto& cell_at) {
        int first = -1;
        for (int i = 0; i < length; ++i) {
            if (!anchor(cell_at(i))) continue;
            if (first >= 0) return std::pair<int, int>{first, i};
            first = i;
        }
        return std::pair<int, int>{-1, -1};
    };
    for (int y = 0; y < grid.height(); ++y) {
        // UID 218522443826106 extends the horizontal pass to all 33
        // rows on split maps. Empty separator rows remain unchanged.
        const auto horizontal = (y < grid.width() || profile.second_half_start_y) ?
            find_pair(grid.width(), [&](int x) { return grid.at(x, y); }) :
            std::pair<int,int>{-1,-1};
        const int offset = profile.second_half_start_y && y >= profile.second_half_start_y - 1
            ? profile.second_half_start_y - 1 : 0;
        for (int x = 0; x < grid.width(); ++x) {
            const auto vertical = find_pair(profile.map_size_y,
                [&](int row) { return grid.at(x, row + offset); });
            const int row = y - offset;
            const bool east_west = x > horizontal.first && x < horizontal.second;
            const bool north_south = x < profile.map_size_y &&
                row > vertical.first && row < vertical.second;
            const auto cell = grid.at(x, y);
            if (cell != 0 && cell != 12 && cell != 13 && cell != 14) continue;
            assert(cell == (east_west && north_south ? 14 :
                            east_west ? 13 : north_south ? 12 : 0));
        }
    }
}

void test_original_poi_random_half_open_boundaries() {
    // Root generate_array_locations: check_X=floor(random(1,Map_size_X))
    // and check_Y=floor(random(1,Map_size_Y)) (random upper exclusive).
    // Array at check_X-1/check_Y-1 can never place a primary POI center
    // in final grid column 15, or in final unsplit grid row 15.
    constexpr std::uint16_t primary[] = {9,5,6,11,15,103,99,50,51};
    constexpr std::uint32_t seeds[] = {
        0u, 1u, 2u, 123456u, 0x10203040u, 0x55667788u, 0xffffffffu
    };
    for(std::uint8_t level=0;level<=5;++level) {
        for(const auto seed:seeds) {
            minidayz::WorldSystem world;
            assert(world.initialize(seed));
            assert(world.generate_procedural_grid(level,seed));
            const auto& grid=world.grid();
            for(const auto code:primary) {
                assert(grid.at(grid.width()-1,0)!=code);
                for(int y=0;y<grid.height();++y)
                    assert(grid.at(grid.width()-1,y)!=code);
                if(grid.height()==16)
                    for(int x=0;x<grid.width();++x)
                        assert(grid.at(x,15)!=code);
            }
        }
    }
}

void test_every_source_level_quota() {
    for (std::uint8_t level = 0; level <= 5; ++level) {
        minidayz::WorldGenerationProfile profile{};
        assert(minidayz::source_world_profile(level, profile));

        constexpr std::uint32_t seeds[] = {
            0u, 1u, 2u, 123456u, 0x10203040u, 0x4d445a33u, 0x55667788u, 0xffffffffu
        };
        for (const auto seed : seeds) {
            minidayz::WorldSystem world;
            assert(world.initialize(seed));
            assert(world.generate_procedural_grid(level, seed));
            assert(world.build_map_markers(level, seed));
            check_generated_road_connections(world.grid(), profile);

            const auto total = [](std::uint8_t first, std::uint8_t second) {
                return static_cast<std::size_t>(first) + second;
            };
            assert(count_code(world.grid(), 9) == total(profile.villages, profile.second_half_villages));
            assert(count_code(world.grid(), 5) == total(profile.militaries, profile.second_half_militaries));
            assert(count_code(world.grid(), 6) == total(profile.cities, profile.second_half_cities));
            assert(count_code(world.grid(), 11) == total(profile.hospitals, profile.second_half_hospitals));
            assert(count_code(world.grid(), 15) == total(profile.firestations, profile.second_half_firestations));
            assert(count_code(world.grid(), 103) == total(profile.chaoshi, profile.second_half_chaoshi));
            // Root generate_array_locations has NO SetAtXY values 99,
            // 50 or 51. Services replace original road Sprite514 markers
            // only after generate_array_tomap, preserving the road grid.
            assert(count_code(world.grid(),99)==0);
            assert(count_code(world.grid(),50)==0);
            assert(count_code(world.grid(),51)==0);
            std::size_t gas=0,cafes=0,shops=0;
            for(std::size_t i=0;i<world.map_marker_count();++i){
                const auto* m=world.map_marker(i);
                assert(m);
                const auto type=m->marker_type;
                if(type!=30&&type!=31&&type!=50&&type!=51&&
                   type!=108&&type!=109)continue;
                // A raw Array road becomes a roadside Mapgen marker.
                // Never delete or replace its road path in Array518.
                assert(m->source_code==12 || m->source_code==13);
                if(type==30||type==31)++gas;
                else if(type==50||type==51)++cafes;
                else ++shops;
                assert(m->source_code==12?
                    (type==30||type==50||type==108):
                    (type==31||type==51||type==109));
            }
            const std::size_t map_halves=level>=4?2u:1u;
            // Some seeds can exhaust original road candidates. Construct
            // PickRandom then skips that repeat; it never invents cells.
            assert(gas<=map_halves*profile.gas_stations);
            assert(cafes<=map_halves*profile.roadside_cafes);
            assert(shops<=map_halves*profile.lite_shop);
            if(profile.gas_stations)assert(gas>0);
            if(profile.roadside_cafes)assert(cafes>0);
            if(profile.lite_shop)assert(shops>0);
            // Source generate_array_tomap special markers must never
            // be confused with raw Array518 POIs. Check all ordinary
            // difficulty levels and both halves of the 16x33 maps.
            std::size_t special32=0,special105=0,military5=0,military70=0;
            for(std::size_t i=0;i<world.map_marker_count();++i){
                const auto* m=world.map_marker(i);
                assert(m);
                const auto type=m->marker_type;
                assert(type!=1000&&type!=2000&&type!=3000&&
                       type!=8000&&type!=10000);
                if(type==32){++special32;assert(m->source_code==0);}
                if(type==105){++special105;assert(m->source_code==0);}
                if(m->source_code==5){
                    assert(type==22||type==5||type==70);
                    if(type==5)++military5;
                    if(type==70)++military70;
                }
            }
            assert(special32==(level==0?0u:level>=4?2u:1u));
            assert(special105==(level>=4?2u:1u));
            const std::size_t expected_military5=
                level==0?0u:level==1?1u:level==2?2u:level==3?3u:6u;
            const std::size_t expected_military70=
                level<=2?0u:level==3?1u:4u;
            assert(military5==expected_military5);
            assert(military70==expected_military70);
            assert(world.map_marker_count() ==
                   static_cast<std::size_t>(profile.map_size_x) * profile.grid_height);
        }
    }
}

// Test the complete five-phase New Game pipeline on EVERY source world.
// Previous quota-only tests could pass while location/event generation on
// later maps failed and returned the user to the boot menu.
void test_source_startup_proposals_precede_materialization() {
    minidayz::WorldSystem world;
    assert(world.begin_source_generation(0x4d444159u,"Map",3));
    assert(world.advance_source_generation()); // Array locations and roads
    assert(world.advance_source_generation()); // Array -> Mapgen514
    assert(world.advance_source_generation()); // generate_mapgen_toelements
    assert(world.generation_phase()==4);
    assert(world.source_ground_patches().empty());
    assert(world.source_scatter_patches().empty());
    assert(world.advance_source_generation()); // crash, bunker, radiation
    assert(world.generation_phase()==5);
    assert(world.source_ground_patches().empty());
    assert(world.advance_source_generation()); // vehicles, then location load
    assert(world.generation_complete());
    assert(world.source_ground_patches().size()==world.map_marker_count());
}

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
        // EVERY original marker eligible for the root-extracted Tilemap230
        // SetTile program must receive a generated patch, at every map level.
        // This catches silent gaps on the extended 16x33 split maps, even
        // when forests and buildings still make the scene appear populated.
        std::size_t eligible_source_scatter=0;
        const std::uint8_t level_mask=std::uint8_t{1} << level;
        for(std::size_t i=0;i<world.map_marker_count();++i){
            const auto* marker=world.map_marker(i);
            assert(marker);
            bool eligible=false;
            for(const auto& rule:minidayz::stage1::kTerrainScatterRules)
                if(rule.marker==marker->marker_type &&
                   (rule.level_mask&level_mask) &&
                   (rule.perk_gate==0 || (rule.perk_gate==1 && (level==4 || world.generation_perk_dao()==5)) ||
                    (rule.perk_gate==2 && !(level==4 || world.generation_perk_dao()==5)))){
                    eligible=true;break;
                }
            if(eligible)++eligible_source_scatter;
        }
        const auto& scatter=world.source_scatter_patches();
        assert(scatter.size()==eligible_source_scatter);
        assert(!scatter.empty());
        for(const auto& patch:scatter){
            assert(patch.marker_index<world.map_marker_count());
            assert(!patch.tiles.empty());
            for(const auto& tile:patch.tiles){
                assert(tile.local_index<35u*35u);
                assert(tile.original_frame<128);
            }
        }
        using minidayz::WorldFeatureKind;
        std::size_t bridges = 0;
        std::size_t bunkers = 0;
        for (std::size_t i = 0; i < world.feature_count(); ++i) {
            const auto& feature = *world.feature(i);
            if (feature.kind == WorldFeatureKind::Bridge) {
                assert(bridges < minidayz::stage1::kBridgeElementSourceCount);
                const auto& spec = minidayz::stage1::kBridgeElementSource[bridges++];
                assert(feature.source_uid == spec.uid);
                assert(feature.source_object == spec.object);
                assert(feature.position.x == spec.x && feature.position.y == spec.y);
            } else if (feature.kind == WorldFeatureKind::Bunker) {
                assert(bunkers < minidayz::stage1::kBunkerSpawnSourceCount);
                const auto& spec = minidayz::stage1::kBunkerSpawnSource[bunkers++];
                assert(feature.source_uid == spec.uid);
                assert(feature.position.x == 2306.0f + spec.x);
                assert(feature.position.y == 17464.0f + spec.y);
                assert(feature.source_value == (static_cast<std::uint32_t>(spec.slot) << 16 | spec.weight));
            }
        }
        assert(bridges == minidayz::stage1::kBridgeElementSourceCount);
        assert(bunkers == minidayz::stage1::kBunkerSpawnSourceCount);
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
        // Region-marker visibility contract: object404 is a magenta
        // achievement city-counter sprite, but its original world logic
        // needs the 1000x1000 placement and UID intact.
        assert(minidayz::world_source::source_world_logic_marker_not_art(404));
        // Root sprites used for collision, spawn markers, opaque editor
        // masks and nonvisual player position are never scenery art.
        for(const auto marker:{181,193,218,345,346,348,508,514,857,984})
            assert(minidayz::world_source::source_world_logic_marker_not_art(
                static_cast<std::uint16_t>(marker)));
        // The neon green original ghost sprites stay available ONLY to the
        // explicitly event-activated construction preview renderer.
        assert(minidayz::world_source::source_world_preview_needs_live_event(571));
        assert(minidayz::world_source::source_world_preview_needs_live_event(573));
        assert(!minidayz::world_source::source_world_preview_needs_live_event(349));
        assert(!minidayz::world_source::source_world_logic_marker_not_art(347));
        assert(!minidayz::world_source::source_world_logic_marker_not_art(321));
        assert(!minidayz::world_source::source_world_logic_marker_not_art(349));
        assert(!minidayz::world_source::source_world_logic_marker_not_art(184));
        std::size_t hidden_city_counters=0;
        // Trace any originally magenta editor/marker sprites which must not
        // become giant visible world overlays. Sprite colors were checked
        // against original root assets/gfx in audit_purple_world_sprites.py.
        for(std::size_t pi=0;pi<world.placement_count();++pi){
            const auto* pc=world.placement(pi);
            if(!pc || pc->opacity<=0.0f || pc->layer>=44)continue;
            if(pc->object==404){
                ++hidden_city_counters;
                assert(pc->layer==41 && pc->width==1000.0f &&
                       pc->height==1000.0f);
                assert(minidayz::world_source::source_world_logic_marker_not_art(
                           pc->object));
                continue; // preserve source placement; never render it
            }
            switch(pc->object){
            case 407: case 400: case 216: case 557:
            case 646: case 225: case 706: case 128:
                std::printf("AZAHAR_PURPLE_CANDIDATE level=%u obj=%u layer=%u w=%.1f h=%.1f alpha=%.2f x=%.1f y=%.1f\\n",
                    static_cast<unsigned>(level),
                    static_cast<unsigned>(pc->object),
                    static_cast<unsigned>(pc->layer),pc->width,pc->height,
                    pc->opacity,pc->x,pc->y);
                break;
            default:break;
            }
        }
        assert(hidden_city_counters>0);
        std::printf("AZAHAR HIDDEN CITY MARKERS level=%u suppressed_magenta_regions=%zu\\n",
                    static_cast<unsigned>(level),hidden_city_counters);
        // Azahar no-building regression: a terrain-only successful map
        // cannot be called generated. Prove that real root category-7
        // house foundations and their indoor/roof companions exist.
        std::size_t visible_house_layers=0;
        std::size_t visible_house_frames=0;
        float nearest_house_from_west=std::numeric_limits<float>::infinity();
        const auto* source_layout=minidayz::stage1::find_layout("Map");
        assert(source_layout);
        const auto layout_index=static_cast<std::uint16_t>(
            source_layout-minidayz::stage1::kLayoutTable);
        std::size_t built_houses=0, built_houses_in_view=0;
        std::size_t house_roofs=0, town_markers=0;
        minidayz::terrain::VisibleSourcePlacementIndex building_visibility;
        building_visibility.rebuild(world);
        std::vector<std::uint32_t> nearby;
        for(std::size_t marker_i=0;marker_i<world.map_marker_count();++marker_i) {
            const auto* marker=world.map_marker(marker_i);
            if(marker && (marker->marker_type==9 || marker->marker_type==4 ||
                          marker->marker_type==16 || marker->marker_type==17 ||
                          marker->marker_type==6 || marker->marker_type==21))
                ++town_markers;
        }
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* placement=world.placement(i);
            if(!placement || !world.bounds().contains({placement->x,placement->y}))
                continue;
            for(const auto& root:minidayz::world_source::kSourceHousePrimarySpawns){
                if(placement->source_uid!=root.event_uid ||
                   placement->object!=root.object)continue;
                ++built_houses;
                const auto* source_layer=minidayz::stage1::find_layer(
                    layout_index,placement->layer);
                const auto* source_object=minidayz::stage1::find_object(
                    placement->object);
                if(source_layer && source_layer->visible &&
                   placement->layer<44 && placement->layer!=32 &&
                   source_object && source_object->type==18 &&
                   placement->opacity>0.0f) {
                    ++visible_house_layers;
                    if(placement->width>0.0f && placement->height>0.0f)
                        ++visible_house_frames;
                }
                nearest_house_from_west=std::min(nearest_house_from_west,
                    std::fabs(placement->x-600.0f));
                building_visibility.query(placement->x-200.0f,
                    placement->y-120.0f,400.0f,240.0f,nearby);
                if(std::find(nearby.begin(),nearby.end(),
                    static_cast<std::uint32_t>(i))!=nearby.end())
                    ++built_houses_in_view;
                break;
            }
            if(placement->object==321 || placement->object==322 ||
               placement->object==323 || placement->object==324 ||
               placement->object==333)++house_roofs;
        }
        std::printf("AZAHAR HOUSE SOURCE level=%u town_markers=%zu original_house_bases=%zu roofs=%zu broadphase_visible=%zu\\n",
            static_cast<unsigned>(level),town_markers,built_houses,
            house_roofs,built_houses_in_view);
        assert(town_markers>0);
        assert(built_houses>0);
        assert(built_houses_in_view==built_houses);
        std::printf("AZAHAR HOUSE DISPLAY level=%u visible_layers=%zu positive_sized_frames=%zu nearest_west_x=%.1f\\n",
            static_cast<unsigned>(level),visible_house_layers,
            visible_house_frames,nearest_house_from_west);
        assert(visible_house_layers==built_houses);
        assert(visible_house_frames==built_houses);
        assert(house_roofs>0);
        // Step12: for ALL six source maps ensure every original active
        // creation-event marker fits, chooses only authored item IDs and
        // survives a no-loss ground snapshot (large levels are denser).
        minidayz::items::GroundLootSystem ground;
        const auto source_loot =
            minidayz::items::source_spawn_initial_building_loot(
                world,ground,world.source_random());
        assert(source_loot.complete());
        assert(source_loot.spawned+source_loot.no_drop+source_loot.skipped==
               source_loot.attempted);
        assert(source_loot.spawned==ground.count());
        minidayz::items::GroundLootSystem copied;
        assert(copied.restore(ground.snapshot()));
        assert(copied.count()==ground.count());
        std::printf("ORIGINAL BUILDING LOOT level=%u authored=%zu items=%zu no_drop=%zu skipped=%zu\\n",
                    static_cast<unsigned>(level),source_loot.attempted,
                    source_loot.spawned,source_loot.no_drop,source_loot.skipped);
    }
}

void test_category4_variant40_building_spawns() {
    minidayz::WorldSystem world;
    assert(world.initialize(0x1234u));
    for(unsigned subtype=0;subtype<7;++subtype){
        const auto first=world.placement_count();
        assert(world.expand_map_element(4,40,subtype,{9000.0f,9000.0f})==1);
        const minidayz::WorldPlacement* base=nullptr;
        const minidayz::WorldPlacement* solid=nullptr;
        for(auto i=first;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(p->object==1345+subtype)base=p;
            if(p->object==348 && p->source_frame==33+subtype)solid=p;
        }
        assert(base && solid);
        assert(solid->parent_runtime_uid==base->runtime_uid);
        assert(solid->source_variable_0==base->runtime_uid);
        assert(solid->width>1.0f && solid->height>1.0f);
    }
}

void test_nested_category4_building_spawns() {
    minidayz::WorldSystem world;
    assert(world.initialize(0x1234u));
    for(unsigned variant=46;variant<=47;++variant){
        const unsigned base_object=1406+variant-46;
        const auto first=world.placement_count();
        assert(world.expand_map_element(4,variant,0,{9000.0f,9000.0f})>0);
        const minidayz::WorldPlacement* base=nullptr;
        const minidayz::WorldPlacement* solid=nullptr;
        const minidayz::WorldPlacement* shadow=nullptr;
        for(auto i=first;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(p->object==base_object)base=p;
            if(p->object==348 && p->source_frame==variant-6)solid=p;
            if(p->object==347)shadow=p;
        }
        assert(base && solid && shadow);
        assert(solid->parent_runtime_uid==base->runtime_uid);
        assert(shadow->parent_runtime_uid==base->runtime_uid);
        assert(solid->source_variable_0==base->runtime_uid);
        assert(shadow->source_variable_0==base->runtime_uid);
        assert(shadow->height>1.0f);
        assert(solid->width>1.0f && solid->height>1.0f);
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


void test_original_constructed_bed_journal_v4() {
    using namespace minidayz;
    constexpr const char* path="minidayz_constructed_beds_v4.bin";
    constexpr const char* temp="minidayz_constructed_beds_v4.tmp";
    WorldSystem original;
    assert(original.generate_from_source(0x20261122u,"Map",0));
    const auto baseline=original.placement_count();
    assert(original.add_source_constructed_bed(8000.0f,6000.0f,0));
    assert(original.add_source_constructed_bed(8070.0f,6000.0f,1));
    assert(original.placement_count()==baseline+2);
    const auto a=original.placement(baseline);
    const auto b=original.placement(baseline+1);
    assert(a && b && a->object==951 && b->object==951);
    assert(a->source_constructed_bed && b->source_constructed_bed);
    assert(a->width==37.0f && a->height==21.0f);
    assert(a->origin_x==0.5f && a->origin_y==0.5f);
    assert(a->source_variable_0==0 && b->source_variable_0==1);
    assert(original.set_placement_sprite_state(baseline,0,1,1.0f));
    // Frame1 is transient original proximity selection, not a separate bed.
    assert(original.dismantle_source_bed(baseline+1));
    assert(original.placement(baseline+1)->opacity==0.0f);
    const auto beds=original.capture_source_constructed_beds();
    assert(beds.size()==2 && beds[0].active && !beds[1].active);
    assert(original.capture_sprite_states().empty());
    assert(persistence::write_world_visual_state(original,0,path,temp));
    WorldSystem restored;
    assert(restored.generate_from_source(0x20261122u,"Map",0));
    assert(persistence::read_world_visual_state(restored,0,path));
    assert(restored.placement_count()==baseline+2);
    const auto recovered=restored.capture_source_constructed_beds();
    assert(recovered.size()==2);
    assert(recovered[0].runtime_uid==beds[0].runtime_uid);
    assert(recovered[1].runtime_uid==beds[1].runtime_uid);
    assert(recovered[0].material_variant==0);
    assert(recovered[1].material_variant==1 && !recovered[1].active);
    assert(restored.placement(baseline)->source_frame==0);
    assert(restored.placement(baseline+1)->opacity==0.0f);
    assert(!restored.restore_source_constructed_beds(beds)); // no duplicates
    const auto before=restored.placement_count();
    assert(!restored.add_source_constructed_bed(8000.0f,6000.0f,2));
    assert(restored.placement_count()==before);
    std::remove(path);
    std::remove(temp);
}

void test_original_box_drop_catalog() {
    using namespace minidayz::items;
    const auto* rice=source_box_category(1);
    assert(rice && rice->event==803253084945045ULL && rice->count==2);
    const auto& first=kSourceBoxDrops[rice->first];
    const auto& second=kSourceBoxDrops[rice->first+1];
    assert(kSourceBoxChoices[first.first]==4 && first.dx==10 && first.dy==5);
    assert(kSourceBoxChoices[second.first]==4 && second.dx==-10 && second.dy==5);
    const auto* mixed=source_box_category(110);
    assert(mixed && mixed->count==4);
    assert(kSourceBoxDrops[mixed->first].choose);
    assert(kSourceBoxDrops[mixed->first].count==6);
    assert(!source_box_category(0));
    assert(!source_box_category(51)); // nested cases have their own dispatch
    // Exercise every first-choice bucket with the same source RNG mapping;
    // all reachable normal branches must generate real source items.
    auto ground=std::make_unique<GroundLootSystem>();
    for(const unsigned category:{51u,52u,53u,999u}){
        for(std::uint32_t seed=0;seed<512;++seed){
            ground->clear();
            std::mt19937 random(seed);
            assert(source_box_default_nested_drops(category,random,*ground,1000,1000));
            assert(ground->count()>0 && ground->count()<=8);
            for(std::size_t i=0;i<ground->count();++i){
                const auto* entry=ground->entry(i);
                assert(entry && source_item_exists(entry->item.item_id));
                assert(std::abs(entry->x-1000.0f)<=10.0f);
                assert(std::abs(entry->y-1000.0f)<=10.0f);
            }
        }
    }
    // Direct quantity actions must preserve stack size, coordinates and order.
    for(unsigned category=5;category<=13;++category){
        ground->clear();
        assert(source_box_direct_quantity_drops(category,*ground,1000,1000));
        std::size_t index=0;
        for(const auto& row:kSourceBoxDirectQuantityDrops){
            if(row.category!=category)continue;
            const auto* entry=ground->entry(index++);
            assert(entry && entry->item.item_id==row.item);
            assert(entry->item.count==row.count);
            assert(entry->x==1000.0f+row.dx && entry->y==1000.0f+row.dy);
        }
        assert(index==ground->count() && index==(category==13?3u:2u));
    }
    // Verify the native ordered dispatch, not just extracted row metadata.
    for(const unsigned category:{14u,15u,16u,17u,37u,38u,39u,40u,41u,42u,111u,112u,116u,1011u}){
        ground->clear();
        assert(source_box_has_mixed_drops(category));
        assert(source_box_mixed_drops(category,*ground,1000,1000));
        std::size_t index=0;
        for(const auto& row:kSourceBoxMixedDrops){
            if(row.category!=category)continue;
            const auto* entry=ground->entry(index++);
            const auto id=row.object?row.item:source_initial_item_after_default_unlocks(row.item);
            assert(entry && entry->item.item_id==id && entry->item.count==row.count);
            assert(entry->x==1000.0f+row.dx && entry->y==1000.0f+row.dy);
        }
        assert(index>0 && index==ground->count());
    }
    // Check exact RNG bucket, order, positions and quantities for all
    // original default-locked 456789 outcomes, including the ten-drop branch.
    bool reached[16]={};
    for(std::uint32_t seed=0;seed<512;++seed){
        ground->clear();
        std::mt19937 actual(seed), expected(seed);
        const auto outcome=1+source_initial_loot_choice_index(expected(),15);
        reached[outcome]=true;
        assert(source_box_456789_drops(actual,*ground,1000,1000));
        assert(actual==expected);
        std::size_t index=0;
        for(const auto& row:kSourceBox456789Drops){
            if(row.category!=outcome)continue;
            const auto* entry=ground->entry(index++);
            const auto id=row.object?row.item:source_initial_item_after_default_unlocks(row.item);
            assert(entry && entry->item.item_id==id && entry->item.count==row.count);
            assert(entry->x==1000.0f+row.dx && entry->y==1000.0f+row.dy);
        }
        assert(index>0 && index==ground->count());
    }
    for(unsigned outcome=1;outcome<=15;++outcome)assert(reached[outcome]);
    ground->clear();
    assert(!source_box_direct_quantity_drops(0,*ground,0,0));
    assert(ground->count()==0);
    std::mt19937 random(7);
    assert(!source_box_default_nested_drops(0,random,*ground,0,0));
    assert(ground->count()==0);
}

void test_original_bunker_container_open_save_replay() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    WorldSystem world;
    assert(world.generate_from_source(0x715d00d0u,"Map",0));
    std::size_t index=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(p && p->object==kSourceBunkerLootbox &&
           p->layer==kSourceInteriorContainerLayer &&
           p->source_animation==0 && p->source_frame==0) {
            index=i;break;
        }
    }
    // Map's original 796 editor fixture is offscreen and invisible;
    // activate that same exact sprite to exercise the event without
    // inventing a world container or modifying the authored layout.
    assert(index<world.placement_count());
    const auto id=world.placement(index)->runtime_uid;
    assert(world.set_placement_sprite_state(index,0,0,1.0f));
    assert(source_bunker_lootbox_closed(*world.placement(index)));
    assert(world.open_source_bunker_lootbox(index));
    const auto* opened=world.placement(index);
    assert(opened && opened->runtime_uid==id);
    assert(opened->source_frame==kSourceBunkerLootboxOpenedFrame);
    assert(opened->source_variable_1==1);
    assert(!source_bunker_lootbox_closed(*opened));
    assert(!world.open_source_bunker_lootbox(index)); // original bool1 latch
    assert(world.advance_source_bunker_lootbox_fade(index,1.0f));
    assert(world.placement(index)->opacity==1.0f);
    assert(world.advance_source_bunker_lootbox_fade(index,1.0f));
    assert(world.placement(index)->opacity==1.0f);
    assert(world.advance_source_bunker_lootbox_fade(index,0.25f));
    assert(world.placement(index)->opacity==0.75f);
    const auto saved=world.capture_sprite_states();
    assert(!saved.empty());
    assert(world.restore_sprite_states({})); // restore original hidden fixture
    const auto* baseline=world.placement(index);
    assert(baseline && baseline->source_frame==0);
    assert(baseline->source_variable_1==0);
    assert(world.restore_sprite_states(saved));
    assert(world.placement(index)->source_frame==1);
    assert(world.placement(index)->source_variable_1==1);
    assert(world.placement(index)->opacity==0.75f);
    assert(world.advance_source_bunker_lootbox_fade(index,0.75f));
    assert(world.placement(index)->opacity==0.0f);
    assert(!world.open_source_bunker_lootbox(index));
    const auto destroyed=world.capture_sprite_states();
    assert(world.restore_sprite_states(destroyed));
    assert(world.placement(index)->opacity==0.0f);
    assert(world.placement(index)->source_variable_1==1);
}

void test_original_building_specific_loot_on_generated_world() {
    using namespace minidayz;
    using namespace minidayz::items;
    WorldSystem world;
    assert(world.generate_from_source(123456u,"Map",0));
    std::size_t authored=0;
    bool category9=false;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* marker=world.placement(i);
        if(!marker || marker->object!=218 ||
           !marker->source_initial_loot_trigger)continue;
        ++authored;
        assert(marker->source_variable_0==marker->source_loot_category);
        assert(marker->source_loot_category>0);
        category9|=marker->source_loot_category==9;
    }
    assert(authored>25);
    assert(category9);
    auto random=world.source_random();
    GroundLootSystem ground;
    const auto counts=source_spawn_initial_building_loot(world,ground,random);
    assert(counts.complete());
    assert(counts.attempted==authored);
    assert(counts.spawned+counts.no_drop+counts.skipped==authored);
    assert(counts.spawned>5 && counts.spawned==ground.count());
    const auto snapshot=ground.snapshot();
    GroundLootSystem restored;
    assert(restored.restore(snapshot));
    assert(restored.count()==ground.count());
    for(std::size_t i=0;i<ground.count();++i){
        const auto* item=ground.entry(i);
        assert(item && source_item_exists(item->item.item_id));
        bool original_position=false;
        for(std::size_t j=0;j<world.placement_count();++j){
            const auto* p=world.placement(j);
            if(p && p->object==218 && p->source_initial_loot_trigger &&
               std::floor(p->x+0.5f)==item->x &&
               std::floor(p->y+0.5f)==item->y){
                original_position=true;break;
            }
        }
        assert(original_position);
        assert(restored.entry(i)->item.item_id==item->item.item_id);
    }
    // Deterministic replay for equal source world RNG state, not per-item
    // guessed seeds. The real scene calls this only for new/traveled worlds.
    GroundLootSystem replay;
    auto again=world.source_random();
    const auto replay_counts=source_spawn_initial_building_loot(world,replay,again);
    assert(replay_counts.spawned==counts.spawned);
    for(std::size_t i=0;i<ground.count();++i) {
        assert(replay.entry(i)->item.item_id==ground.entry(i)->item.item_id);
        assert(replay.entry(i)->x==ground.entry(i)->x);
        assert(replay.entry(i)->y==ground.entry(i)->y);
    }
}

// Screenshot-reproduction seed from the player's actual Manic Emu map.
// All visible original world sprite states must resolve to source-authored
// animation/frame data; a successful .3dsx build does not check this.
// Regression for the original Sprite241 searchable-wreck art.
 // Root category6 Body.SpawnAnotherObject(241) is explicitly on Map
 // Sunset_Dawn layer45, not the browser-only GUI/rain/night overlays.
 // A blanket layer>=44 visibility filter used to discard every trunk,
 // even though the native world source actually generated them.
void test_original_car_overlay_uses_visible_source_layer() {
    using namespace minidayz;
    WorldSystem world{};
    assert(world.generate_from_source(0x4D444159u,"Map",0));
    terrain::VisibleSourcePlacementIndex index{};
    index.rebuild(world);
    const auto* layout=stage1::find_layout("Map");
    assert(layout);
    const auto layout_id=static_cast<std::uint16_t>(
        layout-stage1::kLayoutTable);
    const stage1::LayerDef* sunset=nullptr;
    for(const auto& candidate:stage1::kLayerTable)
        if(candidate.layout==layout_id &&
           std::strcmp(candidate.name,"Sunset_Dawn")==0){
            sunset=&candidate;break;
        }
    assert(sunset && "root original Sunset_Dawn layer missing");
    std::size_t visible_trunks=0;
    std::vector<std::uint32_t> candidates{};
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* p=world.placement(i);
        if(!p||p->object!=241||p->opacity<=0.0f||
           !world.bounds().contains({p->x,p->y}))continue;
        assert(p->layer==sunset->index);
        assert(world_source::source_world_high_layer_gameplay_art(
            p->object,sunset->name));
        index.query(p->x,p->y,1.0f,1.0f,candidates);
        assert(std::binary_search(candidates.begin(),candidates.end(),
            static_cast<std::uint32_t>(i)));
        ++visible_trunks;
    }
    std::fprintf(stderr,
        "SOURCE ORIGINAL LAYER45 CAR TRUNKS INDEXED=%zu\n",visible_trunks);
    assert(visible_trunks>0);
}

void test_reported_world_seed_original_frame_integrity() {
    using namespace minidayz;
    constexpr std::uint32_t kReportedSeed=0x8D45D166u;
    WorldSystem world{};
    assert(world.generate_from_source(kReportedSeed,"Map",0));
    std::size_t visible_art=0,invalid_frames=0,trees=0,cars=0;
    std::size_t family_car_bodies=0,all_car_trunks=0;
    const auto* map=stage1::find_layout("Map");
    assert(map);
    const auto layout_id=static_cast<std::uint16_t>(
        map-stage1::kLayoutTable);
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        const auto* source_layer=p?
            stage1::find_layer(layout_id,p->layer):nullptr;
        if(p && stage1::source_family_contains(1041,p->object))
            ++family_car_bodies;
        if(p && p->object==241)++all_car_trunks;
        if(!p||!source_layer||p->opacity<=0.0f||p->layer==32||
           (p->layer>=44 &&
            !world_source::source_world_high_layer_gameplay_art(
                p->object,source_layer->name))||
           world_source::source_world_logic_marker_not_art(p->object)||
           world_source::source_world_preview_needs_live_event(p->object))
            continue;
        const auto* def=stage1::find_object(p->object);
        const auto* layer=stage1::find_layer(layout_id,p->layer);
        if(!def||def->type!=18||!layer||!layer->visible||
           std::strstr(layer->name,"respawn")||
           std::strstr(layer->name,"_resp")||
           std::strstr(layer->name,"points"))continue;
        ++visible_art;
        const auto animations=stage1::animations_for_object(p->object);
        if(p->source_animation>=animations.size ||
           !animations.data ||
           p->source_frame>=animations.data[p->source_animation].frames) {
            if(invalid_frames<15)std::fprintf(stderr,
                "SOURCE WORLD FRAME MISSING id=%u layer=%u a=%u f=%u uid=%llu\n",
                static_cast<unsigned>(p->object),
                static_cast<unsigned>(p->layer),
                static_cast<unsigned>(p->source_animation),
                static_cast<unsigned>(p->source_frame),
                static_cast<unsigned long long>(p->runtime_uid));
            ++invalid_frames;
        }
        if(p->object==184||p->object==207||p->object==209||
           p->object==210||p->object==576||
           p->object==577||p->object==581)++trees;
        if(p->object==241)++cars;
    }
    std::printf("USER SEED %08X source_art=%zu trees=%zu "
                "car_trunks=%zu invalid_source_frames=%zu placements=%zu\n",
                kReportedSeed,visible_art,trees,cars,
                invalid_frames,world.placement_count());
    std::printf("USER SEED CAR TRUNK SOURCE family1041_bodies=%zu "
                "raw_object241=%zu drawn_candidate241=%zu\n",
                family_car_bodies,all_car_trunks,cars);
    assert(visible_art>100);
    assert(trees>10);
    // Original 241 car trunk is drawn above Trees43 on Sunset_Dawn45.
    // The old index/render filters wrongly treated it as a browser-only
    // night/weather layer and hid every original source car trunk.
    assert(world_source::source_world_high_layer_gameplay_art(241,"Sunset_Dawn"));
    assert(world_source::source_world_high_layer_gameplay_art(1126,"Rain"));
    assert(!world_source::source_world_high_layer_gameplay_art(241,"Rain"));
    assert(!world_source::source_world_high_layer_gameplay_art(241,"night"));
    assert(!world_source::source_world_high_layer_gameplay_art(404,"Sunset_Dawn"));
    // Source var1=3 variants 15/16/17 create DIFFERENT original sprites
    // on high "night" and "Rain" layers. Previously those generated
    // sprites were silently culled by the two-object-only art whitelist.
    // Require their precise original Spawn event, object and layer.
    assert(world_source::source_world_high_layer_gameplay_art(
        1417,"night",611509906059529ULL));
    assert(world_source::source_world_high_layer_gameplay_art(
        1419,"night",969885855510364ULL));
    assert(world_source::source_world_high_layer_gameplay_art(
        1420,"Rain",635923503739935ULL));
    assert(!world_source::source_world_high_layer_gameplay_art(1417,"night"));
    assert(!world_source::source_world_high_layer_gameplay_art(
        1417,"night",969885855510364ULL));
    assert(!world_source::source_world_high_layer_gameplay_art(
        1417,"Rain",611509906059529ULL));
    assert(!world_source::source_world_high_layer_gameplay_art(
        404,"Sunset_Dawn",611509906059529ULL));
    // Keep the source table as the authority for ALL future generated
    // high-layer world sprites, instead of forgetting to add another
    // manually entered exception every time an original frame is found.
    // Never accept unrecognized Construct numeric operators as true:
    // the native executor explicitly supports source 0..5 and wildcard255.
    // All generated root Mapgen predicates must remain inside that set.
    for(const auto& rule:stage1::kWorldElementSpawnSource)
        for(const auto op:{rule.var1_op,rule.var2_op,rule.var3_op,rule.var6_op})
            assert(op<=5 || op==255);
    std::size_t original_high_art=0;
    for(const auto& item:stage1::kWorldElementSpawnSource){
        if(std::strcmp(item.layer_name,"night")!=0 &&
           std::strcmp(item.layer_name,"Rain")!=0)continue;
        assert(world_source::source_world_high_layer_gameplay_art(
            item.object,item.layer_name,item.uid));
        ++original_high_art;
    }
    assert(original_high_art==3);

    assert(invalid_frames==0 && "original world sprite frame not in source");
}

} // namespace

static void test_poi_order_save_compatibility() {
    for (unsigned level : {0u, 1u, 4u, 5u}) {
        minidayz::WorldSystem current, legacy;
        assert(current.begin_source_generation(123456u, "Map", level, 0, false, true));
        assert(legacy.begin_source_generation(123456u, "Map", level, 0, true));
        assert(current.advance_source_generation() && legacy.advance_source_generation());
        unsigned changed = 0;
        for (int y = 0; y < current.grid().height(); ++y) {
            for (int x = 0; x < current.grid().width(); ++x) {
                const auto a = current.grid().at(x, y), b = legacy.grid().at(x, y);
                if (a == b) continue;
                assert((a == 103 && b == 15) || (a == 15 && b == 103));
                ++changed;
            }
        }
        assert((changed > 0) == (level != 0));
    }
}

static void test_original_split_road_save_compatibility() {
    unsigned late_changes = 0;
    for (unsigned level : {0u, 1u, 2u, 3u, 4u, 5u}) {
        for (std::uint32_t seed : {1u, 17u, 123456u, 0x1234u}) {
            minidayz::WorldSystem current, old;
            assert(current.begin_source_generation(seed, "Map", level));
            assert(old.begin_source_generation(seed, "Map", level, 0, false, true));
            assert(current.advance_source_generation() && old.advance_source_generation());
            for (int y = 0; y < current.grid().height(); ++y) {
                for (int x = 0; x < current.grid().width(); ++x) {
                    const auto a = current.grid().at(x, y), b = old.grid().at(x, y);
                    if (a == b) continue;
                    assert(level >= 4 && y >= 17);
                    assert((a == 13 && b == 0) || (a == 14 && b == 12));
                    ++late_changes;
                }
            }
        }
    }
    assert(late_changes > 0);
}

static void test_source_start_event_order() {
    constexpr std::uint32_t seed=123456u;
    for (std::uint8_t level=0;level<=5;++level) {
        std::mt19937 rng(seed);
        const auto unit=[&](){return static_cast<double>(rng())/4294967296.0;};
        minidayz::WorldSystem world;
        assert(world.begin_source_generation(seed,"Map",level,0,false,false,false,false,true));
        const float beach_y=static_cast<float>(1500.0+unit()*(world.bounds().height-5000.0));
        const std::uint8_t themes[]={4,5,7};
        const auto theme=level>4?themes[static_cast<unsigned>(unit()*3)]:0;
        const float expected_x=level>4?static_cast<float>(1000.0+unit()*15500.0):600.0f;
        minidayz::WorldPoint point{};
        assert(!world.source_start_position(point));
        assert(world.advance_source_generation());
        assert(world.generation_perk_dao()==theme);
        assert(world.source_start_position(point));
        assert(point.x==expected_x && point.y==(level>4?373.0f:beach_y));
        auto after_grid=world.source_random();
        minidayz::WorldPoint repeat{};
        assert(world.source_start_position(repeat));
        assert(repeat.x==point.x && repeat.y==point.y);
        assert(world.source_random()==after_grid);
        minidayz::WorldSystem replay;
        assert(replay.begin_source_generation(seed,"Map",level,0,false,false,false,false,true));
        assert(replay.advance_source_generation());
        for(int y=0;y<world.grid().height();++y)
            for(int x=0;x<world.grid().width();++x)
                assert(replay.grid().at(x,y)==world.grid().at(x,y));
        assert(replay.source_random()==world.source_random());
        minidayz::WorldSystem legacy;
        assert(legacy.begin_source_generation(seed,"Map",level));
        assert(legacy.advance_source_generation());
        assert(!legacy.source_start_position(point));
    }
    minidayz::WorldTravel travel;
    assert(travel.begin(seed,5,0,true));
    assert(travel.advance());
    minidayz::WorldPoint destination{};
    assert(travel.destination()->source_start_position(destination));
    assert(destination.y==373.0f);
}

static void test_original_location_reload_selection() {
    minidayz::WorldSystem world;
    assert(world.generate_from_source(123456u,"Map",0));
    const auto h=minidayz::world_source::mapgen_hotspot(3,3);
    minidayz::WorldPoint start{h.x+500.0f,h.y+500.0f};
    const auto snapshot=world.source_random();
    const auto placements=world.placement_count();
    assert(world.start_source_location_reload(start));
    const auto verify=[&](minidayz::WorldPoint actor,double radius){
        unsigned count=0;
        for(std::size_t i=0;i<world.map_marker_count();++i){
            const auto* m=world.map_marker(i);
            const auto h=minidayz::world_source::mapgen_hotspot(m->cell_x,m->cell_y);
            const double dx=actor.x-(h.x+500.0),dy=actor.y-(h.y+500.0);
            const bool active=dx*dx+dy*dy<=radius*radius;
            assert(world.source_location_active(i)==active);
            if(active)++count;
        }
        assert(count>0 && count<world.map_marker_count());
        unsigned linked=0;
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(p->source_reload_marker){
                assert(world.source_placement_active(*p)==
                    world.source_location_active(p->source_reload_marker-1));
                ++linked;
            }else assert(world.source_placement_active(*p));
        }
        assert(linked>0);
    };
    verify(start,2800.0);
    assert(!world.update_source_location_reload({start.x+400.0f,start.y},1.0f));
    assert(!world.update_source_location_reload({start.x+401.0f,start.y},0.5f));
    assert(world.update_source_location_reload({start.x+401.0f,start.y},0.5f));
    verify({start.x+401.0f,start.y},1800.0);
    assert(!world.update_source_location_reload({start.x+801.0f,start.y},1.0f));
    assert(world.update_source_location_reload({start.x+1800.0f,start.y},1.0f));
    verify({start.x+1800.0f,start.y},1800.0);
    assert(world.update_source_location_reload(start,1.0f));
    verify(start,1800.0);
    assert(world.placement_count()==placements && world.source_random()==snapshot);
    assert(world.load_source_layout("Menu"));
    assert(world.source_location_active(0));
}

static void test_original_category3_elements(){
    minidayz::WorldSystem world;
    constexpr std::uint32_t seed=123456u;
    assert(world.begin_source_generation(seed,"Map",0,0,false,false,false,false,true,true));
    auto rng=world.source_random();
    const auto first=world.placement_count();
    const std::uint8_t variants[]={1,3,4,5};
    const std::uint16_t animations[]={0,2,1,3};
    const float widths[]={32,32,28,30},heights[]={23,29,18,21};
    for(unsigned i=0;i<4;++i){
        assert(world.expand_map_element(3,variants[i],0,{1000,2000},0)==1);
        const auto* b=world.placement(first+i);
        assert(b && b->object==296 && b->source_animation==animations[i]);
        assert(b->source_frame==static_cast<unsigned>((static_cast<double>(rng())/4294967296.0)*2.0));
        assert(b->width==widths[i]&&b->height==heights[i]);
    }
    assert(world.source_random()==rng);
    assert(world.expand_map_element(3,3,0,{1000,2000},1)==1);
    assert(world.source_random()==rng);
    for(std::uint8_t variant:{6,8,9,10,11}){
        const auto n=world.placement_count();
        assert(world.expand_map_element(3,variant,0,{1000,2000},0)==1);
        assert(world.placement_count()==n+2);
        const auto* base=world.placement(n);
        const auto* loot=world.placement(n+1);
        assert(base && base->width>0 && base->height>0);
        assert(loot && loot->object==218 && loot->source_initial_loot_trigger);
        assert(loot->source_loot_category==(variant==6?29:37));
        assert(loot->parent_runtime_uid==base->runtime_uid);
        assert(loot->x==base->x && loot->y==base->y);
        assert(world.expand_map_element(3,variant,0,{1000,2000},1)==1);
        assert(world.placement_count()==n+3);
    }
}

static void test_original_category5_elements(){
    minidayz::WorldSystem world;
    assert(world.begin_source_generation(123456u,"Map",0,0,false,false,false,false,true,true,true));
    const auto rng=world.source_random();
    for(std::uint8_t v=0;v<=15;++v){
        const auto first=world.placement_count();
        assert(world.expand_map_element(5,v,0,{1000,2000},0)==1);
        const auto* base=world.placement(first);
        const auto* solid=world.placement(first+1);
        assert(base && base->object==439 && base->source_frame==std::min<unsigned>(v,12));
        assert(solid && solid->object==345 && solid->source_frame==(v<12?23u:27u));
        assert(solid->width==(v<12?5.0f:30.0f));
        assert(solid->x==base->x && solid->y==base->y);
        assert(solid->source_variable_0==base->runtime_uid);
        assert(solid->linked_runtime_uid==base->runtime_uid && base->linked_runtime_uid==solid->runtime_uid);
        assert(world.placement_count()==first+(v>=12&&v<=14?3u:2u));
        if(v>=12&&v<=14){
            const auto* light=world.placement(first+2);
            assert(light->object==663 && light->source_frame==(v==13?0u:1u));
            assert(light->opacity==0.5f);
            assert(light->parent_runtime_uid==base->runtime_uid);
            assert(light->source_variable_0==base->runtime_uid);
            assert(light->scale_x==(v==14?-1.0f:1.0f));
            assert(minidayz::world_source::source_world_high_layer_gameplay_art(
                light->object,"night",light->source_uid));
            const float x[]={986.0f,999.0f,1011.0f};
            const float y[]={1862.0f,1864.0f,1862.0f};
            assert(std::fabs(light->x-x[v-12])<0.01f);
            assert(std::fabs(light->y-y[v-12])<0.01f);
        }
    }
    assert(world.source_random()==rng);
    minidayz::WorldSystem legacy;
    assert(legacy.begin_source_generation(123456u,"Map",0));
    const auto n=legacy.placement_count();
    assert(legacy.expand_map_element(5,12,0,{1000,2000},0)==1);
    assert(legacy.placement_count()==n+1);
}

static void test_original_category46_event_chains(){
    using namespace minidayz;
    WorldSystem world;
    assert(world.begin_source_generation(123456u,"Map",0,0,false,false,false,false,true,true,true,true));
    const auto rng=world.source_random();
    const WorldPoint position{1000,2000};
    for(const auto& c:world_source::kCategory46Cases){
        if(c.category==7)continue;
        const auto first=world.placement_count();
        const auto roots=world.expand_map_element(c.category,c.variant,c.subtype,position,c.state);
        assert(world.placement_count()==first+c.count);
        unsigned expected_roots=0;
        for(unsigned j=0;j<c.count;++j){
            const auto& r=world_source::kCategory46Placements[c.first+j];
            const auto* p=world.placement(first+j);
            assert(p && p->object==r.object && p->source_frame==r.frame);
            assert(std::fabs(p->x-position.x-r.x)<0.001f &&
                   std::fabs(p->y-position.y-r.y)<0.001f);
            assert(p->width==r.w && p->height==r.h && p->origin_x==r.ox &&
                   p->origin_y==r.oy && p->scale_x==r.sx);
            if(r.parent<0)++expected_roots;
            else assert(p->parent_runtime_uid==world.placement(first+r.parent)->runtime_uid);
            const auto value=[&](int v)->std::uint64_t{
                return v<0?world.placement(first+static_cast<unsigned>(-v-1))->runtime_uid:
                    static_cast<std::uint64_t>(v);
            };
            assert(p->source_variable_0==value(r.var0));
            assert(p->source_variable_1==value(r.var1));
            assert(p->source_initial_loot_trigger==r.loot);
        }
        assert(roots==expected_roots);
    }
    // Independently assert concrete root cases: fence frame2 chooses
    // obstacle frame12; category4 variant36's loot uses half-width+15;
    // opened wreck has frame1 and no search marker; fresh wreck mirrors
    // its trunk and links both original children.
    auto n=world.placement_count();
    assert(world.expand_map_element(4,1,2,position,0)==1);
    assert(world.placement(n)->object==319 && world.placement(n)->source_frame==2);
    assert(world.placement(n+1)->object==345 && world.placement(n+1)->source_frame==12);
    n=world.placement_count();
    assert(world.expand_map_element(4,36,0,position,0)==1);
    const auto* loot=world.placement(n+2);
    assert(loot && loot->object==218 && loot->source_loot_category==38);
    assert(loot->source_initial_loot_trigger && std::fabs(loot->x-1043.5f)<0.001f);
    n=world.placement_count();
    assert(world.expand_map_element(6,1,0,position,0)==1);
    assert(world.placement(n+2)->object==241 && world.placement(n+2)->scale_x==-1.0f);
    assert(world.placement(n)->source_variable_0==world.placement(n+1)->runtime_uid);
    assert(world.placement(n)->source_variable_1==world.placement(n+2)->runtime_uid);
    n=world.placement_count();
    assert(world.expand_map_element(6,1,0,position,1)==1);
    assert(world.placement(n)->source_frame==1 && world.placement_count()==n+2);
    assert(world.source_random()==rng);
}

static void test_original_category7_event_chains(){
    using namespace minidayz;
    WorldSystem world;
    assert(world.begin_source_generation(123456u,"Map",0,0,false,false,false,false,true,true,true,true,true));
    const auto rng=world.source_random();
    const WorldPoint pos{1000,2000};
    unsigned fresh=0,used=0;
    for(const auto& c:world_source::kCategory46Cases){
        if(c.category!=7)continue;
        const auto first=world.placement_count();
        const auto roots=world.expand_map_element(7,c.variant,0,pos,c.state);
        if(world.placement_count()!=first+c.count)
            std::fprintf(stderr,"category7 variant %u state %u expected %u placements got %zu\n",
                static_cast<unsigned>(c.variant),static_cast<unsigned>(c.state),
                static_cast<unsigned>(c.count),world.placement_count()-first);
        assert(world.placement_count()==first+c.count);
        unsigned expected=0;
        for(unsigned j=0;j<c.count;++j){
            const auto& r=world_source::kCategory46Placements[c.first+j];
            const auto* p=world.placement(first+j);
            assert(p && p->object==r.object && p->source_frame==r.frame &&
                   p->source_animation==r.animation);
            assert(std::fabs(p->x-pos.x-r.x)<0.001f && std::fabs(p->y-pos.y-r.y)<0.001f);
            assert(p->width==r.w && p->height==r.h && p->origin_x==r.ox && p->origin_y==r.oy);
            if(r.parent<0)++expected;
            else assert(p->parent_runtime_uid==world.placement(first+r.parent)->runtime_uid);
            const auto value=[&](int v)->std::uint64_t{
                return v<0?world.placement(first+static_cast<unsigned>(-v-1))->runtime_uid:
                    static_cast<std::uint64_t>(v);
            };
            assert(p->source_variable_0==value(r.var0) && p->source_variable_1==value(r.var1));
            assert(p->source_variable_2==value(r.var2) && p->source_variable_3==value(r.var3));
            assert(p->source_variable_5==value(r.var5));
            assert(p->source_initial_loot_trigger==r.loot);
        }
        assert(roots==expected);
        if(c.state==0)fresh+=c.count;
        if(c.state==1)used+=c.count;
    }
    assert(fresh==407 && used==254);
    auto n=world.placement_count();
    assert(world.expand_map_element(7,1,0,pos,0)==1);
    assert(world.placement(n+2)->object==218 && world.placement(n+2)->x==1045.5f);
    assert(world.placement(n+2)->y==2000.0f);
    n=world.placement_count();
    assert(world.expand_map_element(7,1,0,pos,1)==1);
    assert(world.placement_count()==n+2);
    n=world.placement_count();
    assert(world.expand_map_element(7,2,0,pos,0)==1);
    assert(world.placement(n)->source_variable_2==world.placement(n+1)->runtime_uid);
    assert(world.placement(n)->source_variable_5==world.placement(n+2)->runtime_uid);
    assert(world.placement(n+1)->source_variable_3==world.placement(n)->runtime_uid);
    assert(world.source_random()==rng);
    // Secret house pools are selected only once, even if two variant35
    // locations were generated in this world. Test separately from ordinary
    // source loot so earlier markers do not obscure the three direct drops.
    WorldSystem secret;
    assert(secret.begin_source_generation(123456u,"Map",0,0,false,false,false,false,true,true,true,true,true));
    assert(secret.expand_map_element(7,35,0,pos,0)==1);
    assert(secret.expand_map_element(7,35,0,{4000,5000},0)==1);
    minidayz::items::GroundLootSystem loot;
    std::mt19937 random(123456u);
    auto summary=minidayz::items::source_spawn_initial_building_loot(secret,loot,random);
    assert(summary.complete() && summary.spawned==3 && loot.count()==3);
    for(unsigned i=0;i<3;++i){
        const auto& pool=world_source::kSecretHouseDrops[i];
        const auto* item=loot.entry(i);
        assert(std::find(pool.items,pool.items+pool.count,item->item.item_id)!=pool.items+pool.count);
        assert(item->x<1100 && item->y<2100);
    }
}

static void test_original_location_transient_recreation(){
    using namespace minidayz;
    WorldSystem world;
    assert(world.begin_source_generation(123456u,"Map",0,0,false,false,false,false,true,true,true,true,true));
    while(!world.generation_complete())assert(world.advance_source_generation());
    // Use actual generated locations, not manually added scenery.
    std::size_t tree=world.placement_count(),door=tree,car=tree;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* p=world.placement(i);
        if(!p || !p->source_reload_marker || !p->linked_runtime_uid)continue;
        if(p->object==184 && p->source_frame==0)tree=i;
        if(p->object==349 && p->source_frame==0)door=i;
        if(p->object==241)car=i;
    }
    assert(tree<world.placement_count() && door<world.placement_count() && car<world.placement_count());
    const auto center=[&](std::size_t i){
        const auto* m=world.map_marker(world.placement(i)->source_reload_marker-1);
        const auto h=world_source::mapgen_hotspot(m->cell_x,m->cell_y);
        return WorldPoint{h.x+500,h.y+500};
    };
    const auto away=[&](WorldPoint a){
        return WorldPoint{a.x<world.bounds().width/2?world.bounds().width-600.0f:600.0f,a.y};
    };
    const auto slot=[](std::uint64_t uid){return static_cast<std::size_t>((uid&~(std::uint64_t{1}<<63))-1);};
    const auto t=*world.placement(tree);
    const auto tcenter=center(tree);
    assert(world.start_source_location_reload(tcenter));
    assert(world.set_placement_sprite_state(tree,0,1,t.opacity));
    assert(world.set_placement_sprite_state(slot(t.linked_runtime_uid),0,10,0));
    assert(world.update_source_location_reload(away(tcenter),1.0f));
    assert(!world.source_placement_active(*world.placement(tree)));
    assert(world.placement(tree)->source_frame==0);
    assert(world.placement(slot(t.linked_runtime_uid))->opacity>0);
    assert(world.update_source_location_reload(tcenter,1.0f));
    assert(world.source_placement_active(*world.placement(tree)));
    const auto d=*world.placement(door);
    const auto dcenter=center(door);
    assert(world.start_source_location_reload(dcenter));
    assert(world.set_placement_sprite_state(door,d.source_animation,1,d.opacity));
    assert(world.placement(slot(d.linked_runtime_uid))->opacity==0);
    assert(world.update_source_location_reload(away(dcenter),1.0f));
    assert(world.placement(door)->source_frame==0);
    assert(world.placement(slot(d.linked_runtime_uid))->opacity>0);
    assert(world.update_source_location_reload(dcenter,1.0f));
    const auto ccenter=center(car);
    assert(world.start_source_location_reload(ccenter));
    assert(world.mark_source_car_trunk_searched(car));
    const auto body=slot(world.placement(car)->parent_runtime_uid);
    assert(world.update_source_location_reload(away(ccenter),1.0f));
    assert(world.update_source_location_reload(ccenter,1.0f));
    assert(world.placement(car)->opacity==0 && world.placement(car)->source_variable_0==1);
    assert(world.placement(body)->source_frame==1);
}

static void test_source_crash_rejection_order(){
    using namespace minidayz;
    std::mt19937 rng(1u),reference=rng;
    std::vector<bool> used{true,false,true,false};
    const std::vector<std::size_t> candidates{0,1,2,3};
    for(unsigned n=0;n<2;++n){
        std::size_t expected;
        do{expected=static_cast<std::size_t>(
            (static_cast<double>(reference())/4294967296.0)*4.0);}
        while(used[expected]);
        assert(world_source::source_pick_unused_crash_marker(candidates,used,rng)==expected);
        assert(used[expected] && rng==reference);
    }
    assert(world_source::source_pick_unused_crash_marker(candidates,used,rng)==used.size());
    assert(rng==reference);
    // Actual generation phase: crash selections precede bunker, radiation
    // and vehicles; use original quota and all marker candidates including
    // previous successful picks, and compare the resulting anchor owners.
    for(std::uint8_t level=0;level<6;++level){
        WorldSystem world;
        assert(world.begin_source_generation(123456u,"Map",level,0,false,false,false,false,true,true,true,true,true,true));
        for(unsigned i=0;i<3;++i)assert(world.advance_source_generation());
        auto expected_rng=world.source_random();
        std::vector<bool> selected(world.map_marker_count(),false);
        std::vector<std::size_t> heli,hammer;
        for(int half=0;half<(level>=4?2:1);++half){
            std::vector<std::size_t> h,v;
            for(std::size_t i=0;i<world.map_marker_count();++i){
                const auto* m=world.map_marker(i);
                if(half==0?m->cell_y>15:m->cell_y<17)continue;
                if(m->marker_type==1 || m->marker_type==2 || m->marker_type==8 ||
                   m->marker_type==10 || m->marker_type==52)h.push_back(i);
                if(m->marker_type==5 || m->marker_type==13 || m->marker_type==15)v.push_back(i);
            }
            const auto run=[&](const auto& candidates,unsigned quota,auto& result){
                for(unsigned n=0;n<quota;++n){
                    bool available=false;
                    for(auto i:candidates)if(!selected[i])available=true;
                    if(!available)break;
                    std::size_t picked;
                    do{picked=candidates[static_cast<std::size_t>(
                        (static_cast<double>(expected_rng())/4294967296.0)*candidates.size())];}
                    while(selected[picked]);
                    selected[picked]=true;result.push_back(picked+1);
                }
            };
            run(h,world_source::source_helicrash_quota(level),heli);
            run(v,world_source::source_hammercrash_quota(level),hammer);
        }
        const auto start=world.placement_count();
        assert(world.advance_source_generation());
        std::vector<std::size_t> actual_heli,actual_hammer;
        for(std::size_t i=start;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            if(p->object==344)actual_heli.push_back(p->source_variable_1);
            if(p->object==434)actual_hammer.push_back(p->house_base_runtime_uid);
        }
        assert(actual_heli==heli && actual_hammer==hammer);
    }
}

static void test_source_first_visit_creation(){
    using namespace minidayz;
    auto world=std::make_unique<WorldSystem>();
    assert(world->begin_source_generation(123456u,"Map",1,0,false,false,false,false,true,true,true,true,true,true,true));
    while(!world->generation_complete())assert(world->advance_source_generation());
    for(const auto& p:world->source_ground_patches())assert(p.source_object==0);
    assert(world->source_scatter_patches().empty());
    WorldPoint start{};
    assert(world->source_start_position(start));
    assert(world->start_source_location_reload(start));
    auto ground=std::make_unique<items::GroundLootSystem>();
    assert(world->execute_source_location_creation(*ground));
    const auto before=world->source_random();
    const auto count=ground->count();
    for(std::size_t i=0;i<world->map_marker_count();++i){
        const auto& patch=world->source_ground_patches()[i];
        assert((patch.source_object!=0)==world->source_location_active(i));
    }
    std::size_t far=world->placement_count();
    for(std::size_t i=0;i<world->placement_count();++i){
        const auto* p=world->placement(i);
        if(p->source_reload_marker && p->source_initial_loot_trigger &&
           world->bounds().contains({p->x,p->y})){
            assert(p->source_initial_loot_consumed==world->source_placement_active(*p));
            if(!world->source_placement_active(*p))far=i;
        }
    }
    assert(far<world->placement_count());
    assert(world->execute_source_location_creation(*ground));
    assert(ground->count()==count && world->source_random()==before);
    const auto* m=world->map_marker(world->placement(far)->source_reload_marker-1);
    const auto h=world_source::mapgen_hotspot(m->cell_x,m->cell_y);
    const WorldPoint next{h.x+500,h.y+500};
    assert(world->update_source_location_reload(next,1.0f));
    assert(world->execute_source_location_creation(*ground));
    assert(world->placement(far)->source_initial_loot_consumed);
    constexpr const char* file="/tmp/mdz_first_visit_world_visual.sav";
    constexpr const char* tmp="/tmp/mdz_first_visit_world_visual.tmp";
    assert(persistence::write_world_visual_state(*world,1,file,tmp));
    auto restored=std::make_unique<WorldSystem>();
    assert(restored->begin_source_generation(123456u,"Map",1,0,false,false,false,false,true,true,true,true,true,true,true));
    while(!restored->generation_complete())assert(restored->advance_source_generation());
    assert(persistence::read_world_visual_state(*restored,1,file));
    assert(restored->placement(far)->source_initial_loot_consumed);
    for(std::size_t i=0;i<world->placement_count();++i)
        assert(restored->placement(i)->source_initial_loot_consumed==world->placement(i)->source_initial_loot_consumed);
    auto existing=std::make_unique<items::GroundLootSystem>();
    assert(existing->restore(ground->snapshot()));
    assert(restored->start_source_location_reload(next));
    assert(restored->execute_source_location_creation(*existing));
    assert(restored->placement(far)->source_initial_loot_consumed);
    std::remove(file);
    std::remove(tmp);
}

static void test_original_tree_overlap_or_guards(){
    using namespace minidayz;
    using namespace minidayz::world_source;
    WorldPlacement tree{},target{};
    tree.x=target.x=1000;tree.y=target.y=2000;
    tree.width=target.width=100;tree.height=target.height=100;
    tree.origin_x=tree.origin_y=target.origin_x=target.origin_y=0.5f;
    tree.scale_x=target.scale_x=1;
    for(std::uint16_t o:{184,207,209,576,577,581}){
        tree.object=o;
        for(std::uint16_t t:{246,430,433}){
            target.object=t;
            assert(source_tree_creation_rejects_overlap(tree,target));
        }
        target.object=345;
        assert(!source_tree_creation_rejects_overlap(tree,target));
    }
    tree.object=210;tree.source_uid=6033785432522481ULL;
    target.object=433;assert(source_tree_creation_rejects_overlap(tree,target));
    target.object=430;assert(!source_tree_creation_rejects_overlap(tree,target));
    target.object=246;assert(source_tree_creation_rejects_overlap(tree,target));
    for(auto uid:{7238212881044891ULL,9881460187187066ULL,1618691315170714ULL}){
        tree.source_uid=uid;
        target.object=430;assert(source_tree_creation_rejects_overlap(tree,target));
        target.object=433;assert(!source_tree_creation_rejects_overlap(tree,target));
        target.object=246;assert(source_tree_creation_rejects_overlap(tree,target));
    }
    target.x+=500;
    assert(!source_tree_creation_rejects_overlap(tree,target));
    // Root frame10 masks must not be silently replaced with rectangles.
    assert(stage1::source_building_collision_polygon(184,0,0));
    assert(stage1::source_building_collision_polygon(246,0,0));
    assert(stage1::source_building_collision_polygon(430,0,0));
    assert(stage1::source_building_collision_polygon(433,0,0));
    WorldSystem world;
    assert(world.source_placement_active(tree));
    tree.source_overlap_rejected=true;
    assert(!world.source_placement_active(tree));
}

int main() {
    test_original_tree_overlap_or_guards();
    test_source_first_visit_creation();
    test_source_crash_rejection_order();
    test_original_location_transient_recreation();
    test_original_category7_event_chains();
    test_original_category46_event_chains();
    test_original_category5_elements();
    test_original_category3_elements();
    test_original_location_reload_selection();
    test_source_start_event_order();
    test_original_split_road_save_compatibility();
    test_poi_order_save_compatibility();
    test_source_startup_proposals_precede_materialization();
    test_original_special_terrain_contexts();
    test_original_high_layer_spawn_visibility();
    test_building_attachment_name_case();
    test_source_furniture_and_pump();
    test_original_constructed_bed_journal_v4();
    test_original_interaction_polygon_geometry();
    test_grid_semantics();
    test_source_world_instance_animation_state();
    test_source_visual_state_snapshot_roundtrip();
    test_original_car_trunk_one_time_search();
    test_original_radiation_site_world_generation();
    test_original_bunker_entrance_generation();
    test_original_source_road_vehicle_placement();
    test_original_crash_site_world_generation();
    test_original_source_settlement_building_groups();
    test_original_tree_block_variants_and_solid_companions();
    test_source_event_tree_hit_health_and_atomic_fall();
    test_source_event_tree_fall_and_door_transition();
    test_original_world_visual_file_roundtrip();
    test_source_tree_fall_item_table();
    test_original_generated_house_companions();
    test_nested_category4_building_spawns();
    test_category4_variant40_building_spawns();
    test_location_state_gates_existing_placement();
    test_original_house_loot_marker_spawns();
    test_original_box_drop_catalog();
    test_original_bunker_container_open_save_replay();
    test_original_building_specific_loot_on_generated_world();
    test_every_source_mapped_house_spawn_chain();
    test_live_original_automatic_doors();
    test_original_roof_family_and_transparency();
    test_generated_object_instance_ids();
    test_source_profiles();
    test_original_map_layout_and_descriptors();
    test_reported_world_seed_original_frame_integrity();
    test_original_car_overlay_uses_visible_source_layer();
    test_seed_reproducibility();
    test_compiled_original_location_terrain_program();
    test_source_scatter_reproduction_and_layout_replacement();
    test_original_perk_dao_world_scatter_branch();
    test_generated_original_exterior_roofs_visible_in_world();
    test_new_game_generates_visible_original_world();
    test_level_four_split_grid();
    test_retry_after_failed_world_travel();
    test_bunker_and_bridge_placement_reload();
    test_original_poi_random_half_open_boundaries();
    test_every_source_level_quota();
    test_full_generation_all_source_maps();
    std::puts("World source regression tests passed.");
    return 0;
}
