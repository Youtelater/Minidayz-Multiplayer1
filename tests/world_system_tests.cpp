#include "minidayz/stage1_data_access.hpp"
#include "minidayz/world.hpp"
#include "minidayz/world_visual_save.hpp"
#include "minidayz/source_world_event_effects.hpp"
#include "minidayz/source_door_auto.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/generated/source_house_companions.hpp"
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
        assert(opacity==(player.y<=threshold?0.25f:1.0f));
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
        assert(exterior->linked_runtime_uid==foundation->runtime_uid);
        assert(world.placement(wall)->linked_runtime_uid==
               foundation->runtime_uid);
        assert(world.placement(shadow)->linked_runtime_uid==
               foundation->runtime_uid);
        assert(world.placement(door)->source_frame==0);
        assert(world.placement(door)->width>0.0f);
        assert(world.placement(door)->height>0.0f);
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
        assert(solid->source_frame==15);
        assert(solid->opacity==1.0f);
        assert(std::fabs(solid->angle-(door->angle+6.0f))<0.001f);
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
    // Generated tree trunks use a separate original collision sprite
    // (object 345) on Obstacle_enviroment, not the visible Trees layer.
    // A spatial index omitting that layer makes the player walk through
    // trunks even when the original sprite is visibly standing.
    const auto* map_layout=minidayz::stage1::find_layout("Map");
    assert(map_layout);
    const auto layout_id=static_cast<std::uint16_t>(
        map_layout-minidayz::stage1::kLayoutTable);
    std::size_t original_obstacle_bases=0;
    for (std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(!p || p->object!=345 || p->width<=0 || p->height<=0)
            continue;
        const auto* layer=minidayz::stage1::find_layer(layout_id,p->layer);
        if(!layer || std::strcmp(layer->name,"Obstacle_enviroment")!=0)
            continue;
        const auto& nearby=collision.candidates(p->x,p->y);
        assert(std::find(nearby.begin(),nearby.end(),
                   static_cast<std::uint32_t>(i))!=nearby.end());
        ++original_obstacle_bases;
    }
    std::printf("ORIGINAL COLLIDER BASES indexed=%zu\\n",
                original_obstacle_bases);


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
    test_source_visual_state_snapshot_roundtrip();
    test_source_event_tree_hit_health_and_atomic_fall();
    test_source_event_tree_fall_and_door_transition();
    test_original_world_visual_file_roundtrip();
    test_source_tree_fall_item_table();
    test_original_generated_house_companions();
    test_live_original_automatic_doors();
    test_original_roof_family_and_transparency();
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
