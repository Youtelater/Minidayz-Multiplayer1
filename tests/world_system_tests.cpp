#include "minidayz/stage1_data_access.hpp"
#include "minidayz/world.hpp"
#include "minidayz/world_visual_save.hpp"
#include "minidayz/source_world_event_effects.hpp"
#include "minidayz/source_world_sprite_visibility.hpp"
#include "minidayz/source_door_auto.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/source_furniture_interaction.hpp"
#include "minidayz/original_initial_building_loot.hpp"
#include "minidayz/generated/source_house_companions.hpp"
#include "minidayz/generated/source_building_frame_geometry.hpp"
#include "minidayz/generated/world_source_data.hpp"
#include "minidayz/source_tree_fall_rules.hpp"
#include "minidayz/terrain_visible_index.hpp"
#include "minidayz/terrain_obstacle_index.hpp"
#include "minidayz/npc_system.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <initializer_list>
#include <utility>
#include <unordered_set>

namespace {

std::size_t count_code(const minidayz::WorldGrid& grid, std::uint16_t code) {
    std::size_t count = 0;
    for (int y = 0; y < grid.height(); ++y)
        for (int x = 0; x < grid.width(); ++x)
            if (grid.at(x, y) == code) ++count;
    return count;
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
    assert(source->source_variable_0>=100 && source->source_variable_0<=200);
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
                if(child.base_object!=action.base_object ||
                   child.variant!=action.variant)continue;
                bool found=false;
                for(std::size_t j=first;j<last;++j){
                    const auto* p=world.placement(j);
                    if(p->object==child.object &&
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
        // Original generate_array_roads_1 only repeats Map_size_X rows;
        // the 33-row late maps do not get a second horizontal pass.
        const auto horizontal = y < grid.width() ?
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
            assert(count_code(world.grid(), 99) == profile.gas_stations);
            assert(count_code(world.grid(), 51) == profile.lite_shop);
            assert(count_code(world.grid(), 50) == profile.roadside_cafes);
            assert(world.map_marker_count() ==
                   static_cast<std::size_t>(profile.map_size_x) * profile.grid_height);
        }
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

} // namespace

int main() {
    test_source_furniture_and_pump();
    test_original_constructed_bed_journal_v4();
    test_original_interaction_polygon_geometry();
    test_grid_semantics();
    test_source_world_instance_animation_state();
    test_source_visual_state_snapshot_roundtrip();
    test_source_event_tree_hit_health_and_atomic_fall();
    test_source_event_tree_fall_and_door_transition();
    test_original_world_visual_file_roundtrip();
    test_source_tree_fall_item_table();
    test_original_generated_house_companions();
    test_nested_category4_building_spawns();
    test_category4_variant40_building_spawns();
    test_location_state_gates_existing_placement();
    test_original_house_loot_marker_spawns();
    test_original_building_specific_loot_on_generated_world();
    test_every_source_mapped_house_spawn_chain();
    test_live_original_automatic_doors();
    test_original_roof_family_and_transparency();
    test_generated_object_instance_ids();
    test_source_profiles();
    test_original_map_layout_and_descriptors();
    test_seed_reproducibility();
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
