#pragma once
#include "minidayz/world.hpp"
#include "minidayz/source_tree_fall_rules.hpp"
#include "minidayz/generated/source_tree_impact_points.hpp"
#include "minidayz/ground_loot.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <random>
#include <string_view>

namespace minidayz::world_source {

// Apply the ORIGINAL tree-fall branch only after its full Construct parent
// conditions have been resolved by combat/collision code. This executor
// deliberately does not guess weapon hit eligibility, HP or RNG. The caller
// must supply the exact, already evaluated Spawn_drop positions in source
// action order (Tree.X+random(-30,30), Tree.Y+40+random(10)).
inline bool apply_verified_tree_fall(
    WorldSystem& world, std::uint64_t runtime_uid,
    std::uint64_t source_event_uid,
    const std::array<WorldPoint,4>& source_drop_positions,
    items::GroundLootSystem& ground,
    const std::array<std::uint16_t,4>* source_drop_counts=nullptr) noexcept {
    if(runtime_uid==0 || !world.generation_complete())return false;
    std::size_t index=world.placement_count();
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(p && p->runtime_uid==runtime_uid) {index=i;break;}
    }
    if(index==world.placement_count())return false;
    const auto* placed=world.placement(index);
    const auto* rule=tree_fall_rule(placed->object);
    if(!rule || source_event_uid!=rule->event_uid ||
       placed->source_animation!=rule->fall_animation ||
       placed->source_frame==rule->fall_frame)return false;
    if(ground.count()+rule->count > items::kMaximumGroundLootItems)
        return false;
    for(std::size_t i=0;i<rule->count;++i)
        if(!std::isfinite(source_drop_positions[i].x) ||
           !std::isfinite(source_drop_positions[i].y))
            return false;
    const auto original=*placed;
    // The six original tree-creation branches bind each tree to an
    // obst_base (object 345), initialized to frame 10. Its companion
    // frame changes to 8 only AFTER Spawn_drop calls complete.
    std::size_t base_index=world.placement_count();
    WorldPlacement original_base{};
    if(original.linked_runtime_uid!=0){
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* candidate=world.placement(i);
            if(candidate && candidate->runtime_uid==original.linked_runtime_uid){
                if(candidate->object!=kOriginalTreeHitboxObject ||
                   candidate->linked_runtime_uid!=runtime_uid ||
                   candidate->source_frame!=kOriginalTreeStandingHitboxFrame)
                    return false;
                base_index=i;
                original_base=*candidate;
                break;
            }
        }
        if(base_index==world.placement_count())return false;
    }
    // GroundLootSystem::add is validated by its own original-item catalog.
    // Save its previous state so a failed drop cannot leave a chopped
    // tree with missing resources.
    // This transaction is too large for the ARM11 main thread stack.
    const auto ground_before=std::unique_ptr<items::GroundLootSystem>(
        new(std::nothrow) items::GroundLootSystem(ground));
    if(!ground_before)return false;
    if(!world.set_placement_sprite_state(index,rule->fall_animation,
                                          rule->fall_frame,original.opacity))
        return false;
    for(std::size_t i=0;i<rule->count;++i) {
        const auto& where=source_drop_positions[i];
        const auto count=source_drop_counts?(*source_drop_counts)[i]:std::uint16_t{1};
        if(!ground.add({rule->drop_item_ids[i],count,
                        items::kUseSourceItemCondition},where.x,where.y)) {
            ground=*ground_before;
            (void)world.set_placement_sprite_state(
                index,original.source_animation,
                original.source_frame,original.opacity);
            return false;
        }
    }
    if(base_index<world.placement_count() &&
       !world.set_placement_sprite_state(
           base_index,original_base.source_animation,
           kOriginalTreeFallenHitboxFrame,original_base.opacity)){
        ground=*ground_before;
        (void)world.set_placement_sprite_state(
            index,original.source_animation,
            original.source_frame,original.opacity);
        return false;
    }
    return true;
}

// Source-gated execution of the original "subtract 1, then <=0" tree hit
// event chain. The caller MUST have evaluated the original ancestor
// conditions (object 345 and bullet 1031 guard) and provide instance
// variable 0 from that particular tree. We do not invent initial tree HP,
// hits, weapon eligibility, RNG, sounds, or spawn offsets.
// A failed fall must NOT consume HP or create a partial item drop.
enum class TreeHitOutcome : std::uint8_t { Rejected, Damaged, Fallen };

inline TreeHitOutcome apply_verified_tree_hit(
    WorldSystem& world, std::uint64_t runtime_uid,
    std::uint64_t source_hit_event_uid,
    float& original_tree_health_variable_0,
    const std::array<WorldPoint,4>& source_drop_positions,
    items::GroundLootSystem& ground,
    const std::array<std::uint16_t,4>* source_drop_counts=nullptr) noexcept {
    if (!world.generation_complete() || runtime_uid==0 ||
        !std::isfinite(original_tree_health_variable_0))
        return TreeHitOutcome::Rejected;

    const WorldPlacement* placed=nullptr;
    for (std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if (p && p->runtime_uid==runtime_uid) { placed=p;break; }
    }
    if (!placed) return TreeHitOutcome::Rejected;
    const auto* hit=tree_hit_rule(placed->object);
    if (!hit || hit->hit_event_uid!=source_hit_event_uid ||
        placed->source_animation!=0 || placed->source_frame!=0)
        return TreeHitOutcome::Rejected;

    // Original Sprite subtract-from-instance-variable 0 action is 1.
    // The fall child evaluates the updated value, without rounding.
    const float next_hp=original_tree_health_variable_0-1.0f;
    if (!std::isfinite(next_hp))return TreeHitOutcome::Rejected;
    if (next_hp>0.0f) {
        original_tree_health_variable_0=next_hp;
        return TreeHitOutcome::Damaged;
    }

    // Original Spawn_drop expressions must already have been evaluated
    // in the same RNG order as the source actions by the caller.
    if (!apply_verified_tree_fall(
            world,runtime_uid,hit->fall_event_uid,
            source_drop_positions,ground,source_drop_counts))
        return TreeHitOutcome::Rejected;
    original_tree_health_variable_0=next_hp;
    return TreeHitOutcome::Fallen;
}

// Apply the source tree hit to the tree selected by its actual linked
// standing obst_base 345. The caller still owns bullet 1031's original
// not-fire, overlap and flag7 guards and sound/tutorial/achievement actions.
// Persist the hit in that tree's variable 0 rather than a caller-local HP.
inline TreeHitOutcome apply_verified_tree_hit_at_trunk(
    WorldSystem& world,std::size_t trunk_index,
    std::uint64_t source_hit_event_uid,
    const std::array<WorldPoint,4>& source_drop_positions,
    items::GroundLootSystem& ground,
    const std::array<std::uint16_t,4>* source_drop_counts=nullptr) noexcept {
    const auto* trunk=world.placement(trunk_index);
    if(!trunk || trunk->object!=kOriginalTreeHitboxObject ||
       trunk->source_frame!=kOriginalTreeStandingHitboxFrame ||
       !world.source_placement_active(*trunk) ||
       !trunk->linked_runtime_uid)return TreeHitOutcome::Rejected;
    const auto uid=trunk->linked_runtime_uid;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* tree=world.placement(i);
        if(!tree || tree->runtime_uid!=uid)continue;
        if(tree->linked_runtime_uid!=trunk->runtime_uid ||
           !world.source_placement_active(*tree) ||
           tree->source_variable_0==0 || tree->source_variable_0>3)
            return TreeHitOutcome::Rejected;
        float hp=static_cast<float>(tree->source_variable_0);
        const auto outcome=apply_verified_tree_hit(world,uid,
            source_hit_event_uid,hp,source_drop_positions,ground,source_drop_counts);
        if(outcome!=TreeHitOutcome::Rejected)
            (void)world.set_source_tree_health(i,static_cast<std::uint64_t>(hp));
        return outcome;
    }
    return TreeHitOutcome::Rejected;
}

// Game_events 5202978245239527 / 8407650361789881:
// the original projectile 1031 must not have type "fire", must collide
// with 345 and must have boolean instance variable 7 set. Collision
// selection is supplied by the projectile engine; a nearby tree alone
// is never a qualifying hit.
struct SourceTreeProjectileCollision {
    std::uint16_t object{};
    std::string_view type{};
    bool flag7{};
    bool collided_with_selected_trunk{};
};
struct SourceTreeCollisionResult {
    TreeHitOutcome outcome{TreeHitOutcome::Rejected};
    const char* chop_sound{};
    const char* fall_sound{};
    bool tutor_tree_chopped{};
};
inline SourceTreeCollisionResult apply_tree_trunk_contact(
    WorldSystem& world,std::size_t trunk_index,
    items::GroundLootSystem& ground,std::mt19937& rng) noexcept {
    const auto* trunk=world.placement(trunk_index);
    if(!trunk || trunk->object!=kOriginalTreeHitboxObject ||
       trunk->source_frame!=kOriginalTreeStandingHitboxFrame ||
       !world.source_placement_active(*trunk) || !trunk->linked_runtime_uid)
        return {};
    const WorldPlacement* tree=nullptr;
    for(std::size_t i=0;i<world.placement_count();++i){
        const auto* candidate=world.placement(i);
        if(candidate && candidate->runtime_uid==trunk->linked_runtime_uid){
            tree=candidate;break;
        }
    }
    if(!tree || tree->linked_runtime_uid!=trunk->runtime_uid ||
       !world.source_placement_active(*tree) ||
       tree->source_variable_0==0 || tree->source_variable_0>3)
        return {};
    const auto* hit=tree_hit_rule(tree->object);
    const auto* fall=tree_fall_rule(tree->object);
    if(!hit || !fall)return {};
    // Preserve the original evaluation order: choose chop sound for
    // every hit; only the zero-HP child chooses a fall sound and evaluates
    // X+random(-30,30), Y+40+random(10) for each ordered Spawn_drop.
    // A failed native transaction leaves both the RNG and HP untouched.
    auto next=std::unique_ptr<std::mt19937>(new(std::nothrow) std::mt19937(rng));
    if(!next)return {};
    const auto unit=[&]() noexcept {
        return static_cast<double>((*next)())/4294967296.0;
    };
    constexpr const char* chop[]={"chop","chop2","chop3","chop4"};
    constexpr const char* fall_sounds[]={"treefall_02","treefall_03"};
    SourceTreeCollisionResult result{};
    result.chop_sound=chop[static_cast<std::size_t>(unit()*4.0)];
    std::array<WorldPoint,4> positions{};
    std::array<std::uint16_t,4> counts{};
    if(tree->source_variable_0==1){
        result.fall_sound=fall_sounds[static_cast<std::size_t>(unit()*2.0)];
        for(std::size_t i=0;i<fall->count;++i){
            positions[i].x=static_cast<float>(tree->x-30.0+60.0*unit());
            positions[i].y=static_cast<float>(tree->y+40.0+10.0*unit());
            // Spawn_drop item 38 -> t384 variable 1 choose(1,2,3,4,5),
            // action 8478952837871408. Its draw happens before the next
            // Spawn_drop coordinate expressions. Items 37/50 only Create.
            counts[i]=fall->drop_item_ids[i]==38
                ? static_cast<std::uint16_t>(1+static_cast<unsigned>(unit()*5.0))
                : std::uint16_t{1};
        }
    }
    result.outcome=apply_verified_tree_hit_at_trunk(
        world,trunk_index,hit->hit_event_uid,positions,ground,&counts);
    if(result.outcome==TreeHitOutcome::Rejected)return {};
    rng=*next;
    result.tutor_tree_chopped=result.outcome==TreeHitOutcome::Fallen;
    return result;
}

inline SourceTreeCollisionResult dispatch_source_tree_projectile_collision(
    WorldSystem& world,std::size_t trunk_index,
    const SourceTreeProjectileCollision& projectile,
    items::GroundLootSystem& ground,std::mt19937& rng) noexcept {
    if(projectile.object!=1031 || projectile.type=="fire" ||
       !projectile.flag7 || !projectile.collided_with_selected_trunk)
        return {};
    return apply_tree_trunk_contact(world,trunk_index,ground,rng);
}

// Original automatic door event UIDs:
// 6061166023614368 sets door object 349 frame 1 (open);
// 243854400993751 sets frame 0 (close).
// Collision/shadow/sound companion actions are NOT implied by this helper;
// the caller must dispatch the full verified original event group.
inline bool apply_verified_door_frame_at_index(
    WorldSystem& world,std::size_t index,std::uint64_t runtime_uid,
    std::uint64_t source_event_uid) noexcept {
    // Both original door event families set the SAME frame and linked
    // Solid 348 state. The second branch also destroys a separate
    // interaction prompt object 506; that additional action belongs to
    // the caller, not this strictly frame-only helper.
    constexpr std::uint64_t kOpen=6061166023614368ULL;
    constexpr std::uint64_t kClose=243854400993751ULL;
    constexpr std::uint64_t kPromptOpen=7258332154939593ULL;
    constexpr std::uint64_t kPromptClose=353730214514351ULL;
    const bool opening=source_event_uid==kOpen ||
                       source_event_uid==kPromptOpen;
    const bool closing=source_event_uid==kClose ||
                       source_event_uid==kPromptClose;
    if(!world.generation_complete() || runtime_uid==0 ||
       (!opening && !closing))return false;
    const auto* p=world.placement(index);
    if(!p || p->runtime_uid!=runtime_uid || p->object!=349)return false;
    const std::uint32_t frame=opening?1u:0u;
    if(p->source_frame==frame)return false;
    return world.set_placement_sprite_state(
        index,p->source_animation,frame,p->opacity);
}

inline bool apply_verified_door_frame(
    WorldSystem& world,std::uint64_t runtime_uid,
    std::uint64_t source_event_uid) noexcept {
    if(!world.generation_complete() || runtime_uid==0)return false;
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* p=world.placement(i);
        if(p && p->runtime_uid==runtime_uid)
            return apply_verified_door_frame_at_index(
                world,i,runtime_uid,source_event_uid);
    }
    return false;
}

} // namespace minidayz::world_source
