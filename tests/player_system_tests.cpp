#include "minidayz/player.hpp"
#include "minidayz/source_player_layers.hpp"
#include "minidayz/source_player_altmove.hpp"
#include "minidayz/source_equipment_animation.hpp"
#include "minidayz/source_energy_drink_boost.hpp"
#include "minidayz/source_player_base_geometry.hpp"
#include "minidayz/generated/stage1_data.hpp"
#include "minidayz/terrain_source_collision.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <initializer_list>
#include <cmath>
#include <limits>

using namespace minidayz;

int main() {
    // Preserve exact original layer relationships among player/equipment
    // (32..39), building backgrounds (31) and building roofs/walls (41).
    using namespace minidayz::stage1;
    assert(kSourcePlayerLayer==32 && kSourcePlayerWeaponLayer==39);
    for(std::uint16_t i=31;i<41;++i)
        assert(source_player_layer_depth(i) <
               source_player_layer_depth(i+1));

    // Original wearable sprites run with their OWN animation speed, loop
    // flag and number of frames. They must not reuse the skin's current
    // integer frame when the source speeds/frame counts differ.
    const minidayz::stage1::AnimationDef independent_loop{
        679, "run_down", 5, 8.0f, true};
    assert(source_animation_frame_at_time(independent_loop,0.0f)==0);
    assert(source_animation_frame_at_time(independent_loop,0.32f)==2);
    assert(source_animation_frame_at_time(independent_loop,0.75f)==1);
    const minidayz::stage1::AnimationDef independent_once{
        679, "pistol_down", 4, 8.0f, false};
    assert(source_animation_frame_at_time(independent_once,100.0f)==3);
    assert(source_animation_frame_at_time(independent_once,-1.0f)==0);
    assert(source_animation_frame_at_time(
        independent_loop,std::numeric_limits<float>::infinity())==0);
    assert(source_equipment_animation_frame(679,"not_an_original_pose",1.0f)==0);
    std::size_t original_animation_checks=0;
    for(const auto& a : kAnimations) {
        if(a.frames<2 || !(a.speed>0.0f) ||
           !std::isfinite(a.speed))continue;
        const float one_frame=1.25f/a.speed;
        assert(source_equipment_animation_frame(a.object,a.name,one_frame)==1);
        assert(source_equipment_animation_frame(a.object,a.name,0.0f)==0);
        ++original_animation_checks;
        if(original_animation_checks==24)break;
    }
    assert(original_animation_checks==24);

    // Original source item_activate(63) starts Timer "Edrink" for 60s
    // and adds +15 to PlayerBase variable4; timer-end resets to zero.
    SourceEnergyDrinkBoost energy{};
    assert(energy.bonus()==0.0f);
    assert(!energy.on_successful_item_use(63,true,true));
    assert(!energy.on_successful_item_use(63,false,false));
    assert(!energy.on_successful_item_use(49,false,true));
    assert(energy.on_successful_item_use(63,false,true));
    assert(energy.bonus()==15.0f && energy.remaining()==60.0f);
    energy.update(59.75f);
    assert(energy.bonus()==15.0f && energy.remaining()==0.25f);
    assert(energy.on_successful_item_use(63,false,true)); // original Timer.Start restarts
    assert(energy.remaining()==60.0f);
    energy.update(60.0f);
    assert(energy.bonus()==0.0f && energy.remaining()==0.0f);
    assert(!energy.restore(61.0f) && !energy.restore(-1.0f));
    assert(energy.restore(24.0f) && energy.bonus()==15.0f);
    assert(energy.restore(0.0f) && energy.bonus()==0.0f);

    // Exact Construct PlayerBase193 AltMove source expression:
    // 100-var6-var7+var4+var3+var5+var8, before native collision.
    minidayz::stage1::SourcePlayerAltMoveModifiers v{};
    assert(minidayz::stage1::source_player_altmove_max_speed(v)==100.0f);
    v.runner_bonus=5.0f;
    assert(minidayz::stage1::source_player_altmove_max_speed(v)==105.0f);
    v.runner_bonus=15.0f; v.regen_bonus=10.0f; v.energy_boost=15.0f;
    assert(minidayz::stage1::source_player_altmove_max_speed(v)==140.0f);
    v.damage_slowdown=50.0f; v.terrain_slowdown=45.0f;
    assert(minidayz::stage1::source_player_altmove_max_speed(v)==45.0f);
    v.damage_slowdown=200.0f;
    assert(minidayz::stage1::source_player_altmove_max_speed(v)==0.0f);
    const auto novice_rules = source_survival_rules(SurvivalDifficulty::novice);
    const auto regular_rules = source_survival_rules(SurvivalDifficulty::regular);
    const auto veteran_rules = source_survival_rules(SurvivalDifficulty::veteran);
    assert(novice_rules.hunger_drain_per_second == 0.1f);
    assert(novice_rules.thirst_drain_per_second == 0.1f);
    assert(regular_rules.hunger_drain_per_second == 0.15f);
    assert(regular_rules.thirst_drain_per_second == 0.15f);
    assert(veteran_rules.hunger_drain_per_second == 0.2f);
    assert(veteran_rules.thirst_drain_per_second == 0.2f);
    PlayerState state{};
    state.health = 5.0f;
    state.hunger = 0.0f;
    state.thirst = 10.0f;
    PlayerSystem starvation(state);

    PlayerRules rules{};
    rules.max_health = 100.0f;
    rules.max_hunger = 100.0f;
    rules.max_thirst = 100.0f;
    rules.starvation_damage_per_second = 50.0f;

    const auto starvation_result = starvation.update_survival(0.1f, rules);
    assert(starvation_result.died);
    assert(!starvation.state().alive);
    assert(starvation.state().health == 0.0f);
    assert(starvation.state().death_reason == PlayerDeathReason::hunger);

    state.health = 5.0f;
    state.hunger = 10.0f;
    state.thirst = 0.0f;
    PlayerSystem dehydration(state);
    rules.starvation_damage_per_second = 0.0f;
    rules.dehydration_damage_per_second = 50.0f;

    const auto dehydration_result = dehydration.update_survival(0.1f, rules);
    assert(dehydration_result.died);
    assert(dehydration.state().death_reason == PlayerDeathReason::thirst);

    state.health = 5.0f;
    PlayerSystem ordinary_damage(state);
    ordinary_damage.apply_damage(10.0f);
    assert(!ordinary_damage.state().alive);
    assert(ordinary_damage.state().death_reason == PlayerDeathReason::other);

    state.health = 5.0f;
    PlayerSystem explicit_cause(state);
    explicit_cause.apply_damage(10.0f, PlayerDeathReason::cold);
    assert(!explicit_cause.state().alive);
    assert(explicit_cause.state().death_reason == PlayerDeathReason::cold);

    
    // Movement API coverage: block X while allowing Y-axis wall sliding.
    PlayerState moving_state{};
    moving_state.health = 100.0f;
    PlayerSystem moving(moving_state);
    PlayerRules movement_rules{};
    movement_rules.walk_speed = 100.0f;
    PlayerInput diagonal{};
    diagonal.move_x = 1.0f;
    diagonal.move_y = 1.0f;
    const auto blocked_x = +[](void*, float x, float) noexcept { return x > 0.0f; };
    const auto move_result = moving.update_movement(diagonal, 0.05f,
        movement_rules, blocked_x, nullptr);
    assert(move_result.blocked_x && !move_result.blocked_y);
    assert(move_result.dx == 0.0f && move_result.dy > 0.0f);
    assert(moving.state().x == 0.0f && moving.state().y > 0.0f);
    assert(moving.state().animation == PlayerAnimation::walk);

    // 10 FPS must still advance 100 ms of real time, not the 50 ms
    // used by the old preview clamp (which halved player speed).
    PlayerState slow_frame_state{};
    slow_frame_state.health=100.0f;
    PlayerSystem slow_frame(slow_frame_state);
    PlayerInput east{};
    east.move_x=1.0f;
    slow_frame.update_movement(east,0.10f,movement_rules);
    assert(std::fabs(slow_frame.state().x-10.0f)<0.001f);
    // A slower 5 FPS emulator frame must advance the same real-world
    // movement as four 50ms frames; never discard the excess 100ms.
    PlayerState one_slow_state{};
    one_slow_state.health=100.0f;
    PlayerSystem one_slow(one_slow_state);
    one_slow.update_movement(east,0.20f,movement_rules);
    PlayerSystem four_fast(one_slow_state);
    for(int i=0;i<4;++i)
        four_fast.update_movement(east,0.05f,movement_rules);
    assert(std::fabs(one_slow.state().x-20.0f)<0.001f);
    assert(std::fabs(one_slow.state().x-four_fast.state().x)<0.001f);


    // The default player skin has idle down/up/right/left (0..3) and
    // run down/up/right/left (4..7); it has no dedicated death animation.
    PlayerState animation_state{};
    animation_state.health = 100.0f;
    PlayerSystem animated(animation_state);
    assert(animated.source_animation_index() == 0);
    PlayerInput up{};
    up.move_y = -1.0f;
    animated.update_movement(up, 0.05f, movement_rules);
    assert(animated.source_animation_index() == 5);
    assert(animated.source_animation_frame() == 0);
    for (int i = 0; i < 20; ++i)
        animated.update_movement(up, 0.05f, movement_rules);
    assert(animated.source_animation_frame() == 1);
    PlayerInput idle{};
    animated.update_movement(idle, 0.05f, movement_rules);
    assert(animated.source_animation_index() == 1);
    assert(animated.source_animation_frame() == 0);
    animated.kill(PlayerDeathReason::other);
    assert(animated.source_animation_index() == 0);
    assert(animated.source_animation_frame() == 0);

    // Source PlayerBase193 has a 2x2 collider anchored at its bottom-right
    // hotspot. Its visible 30x30 skin is NOT its movement hitbox.
    assert(stage1::kSourcePlayerBaseHalfWidth==1.0f);
    assert(stage1::kSourcePlayerBaseHalfHeight==1.0f);
    PlayerState anchor_state{};anchor_state.x=10.0f;anchor_state.y=10.0f;
    anchor_state.health=100.0f;
    PlayerSystem anchored(anchor_state);
    PlayerRules anchor_rules{};anchor_rules.walk_speed=20.0f;
    anchor_rules.collision_half_width=stage1::kSourcePlayerBaseHalfWidth;
    anchor_rules.collision_half_height=stage1::kSourcePlayerBaseHalfHeight;
    anchor_rules.collision_offset_x=stage1::kSourcePlayerBaseCollisionOffsetX;
    anchor_rules.collision_offset_y=stage1::kSourcePlayerBaseCollisionOffsetY;
    float last_probe_x=0.0f,last_probe_y=0.0f;
    struct Probe { float* x;float* y; } probe{&last_probe_x,&last_probe_y};
    const auto capture=+[](void* ctx,float x,float y) noexcept -> bool {
        auto* out=static_cast<Probe*>(ctx);
        *out->x=x;*out->y=y;return false;
    };
    PlayerInput walking_right{};walking_right.move_x=1.0f;
    anchored.update_movement(walking_right,0.05f,anchor_rules,capture,&probe);
    assert(std::fabs(anchored.state().x-11.0f)<0.0001f);
    // Last corner probe uses source rect [x-2,x] at player anchor x=11.
    assert(std::fabs(last_probe_x-11.0f)<0.0001f);
    assert(std::fabs(last_probe_y-10.0f)<0.0001f);

    // The configured player half-size participates in collision checks.
    PlayerState edge_state{};
    edge_state.x = 1.0f;
    edge_state.y = 10.0f;
    edge_state.health = 100.0f;
    PlayerSystem edge_player(edge_state);
    PlayerRules edge_rules{};
    edge_rules.walk_speed = 20.0f;
    edge_rules.collision_half_width = 2.0f;
    edge_rules.collision_half_height = 2.0f;
    const auto outside_world = +[](void*, float x, float y) noexcept {
        return x < 0.0f || y < 0.0f || x > 100.0f || y > 100.0f;
    };
    PlayerInput left{};
    left.move_x = -1.0f;
    const auto edge_move = edge_player.update_movement(
        left, 0.05f, edge_rules, outside_world, nullptr);
    assert(edge_move.blocked_x);
    assert(edge_player.state().x == 1.0f);

    // A thin obstacle between the current position and destination must
    // stop movement even when the unstepped endpoint lies beyond it.
    PlayerState tunnel_state{};
    tunnel_state.health=100.0f;
    tunnel_state.x=10.0f;
    tunnel_state.y=20.0f;
    PlayerSystem tunnel(tunnel_state);
    PlayerRules tunnel_rules{};
    tunnel_rules.run_speed=600.0f;
    tunnel_rules.collision_half_width=1.0f;
    tunnel_rules.collision_half_height=1.0f;
    const auto narrow_wall=+[](void*,float x,float) noexcept {
        return x>=23.0f && x<=26.0f;
    };
    PlayerInput sprint{};
    sprint.move_x=1.0f;
    sprint.run=true;
    const auto stop=tunnel.update_movement(
        sprint,0.05f,tunnel_rules,narrow_wall,nullptr);
    assert(stop.blocked_x);
    assert(tunnel.state().x<23.0f);
    assert(tunnel.state().animation==PlayerAnimation::run);
    // Changing direction resets the old run animation frame: a new
    // directional sprite should never inherit a mismatched pose.
    PlayerState turn_state{};
    turn_state.health=100.0f;
    PlayerSystem turning(turn_state);
    for(int i=0;i<21;++i)
        turning.update_movement(up,0.05f,movement_rules);
    assert(turning.source_animation_frame()==1);
    PlayerInput right{};
    right.move_x=1.0f;
    turning.update_movement(right,0.05f,movement_rules);
    assert(turning.source_animation_index()==6);
    assert(turning.source_animation_frame()==0);
    // Switching to another original player skin preserves physical stats
    // while using the newly selected skin's own animation timing.
    turning.set_source_skin(826); // Original player_skin_aika.
    assert(turning.source_skin()==826);
    assert(turning.source_animation_frame()==0);
    assert(turning.state().health==100.0f);
    turning.set_source_skin(999); // Original player_skin_pilot.
    assert(turning.source_skin()==999);
    turning.set_source_skin(65535); // Invalid object cannot replace skin.
    assert(turning.source_skin()==999);

    // Every original selectable survivor must resolve eight REAL animation
    // indices from its own Construct object, never guessed 0..7 offsets.
    // These IDs come from the source-verified skin catalog, not PNG guesses.
    for (const std::uint16_t skin : {
             679, 826, 743, 1262, 745, 742, 680, 748, 750, 998,
             997, 825, 811, 749, 1009, 999, 741, 996, 746, 747}) {
        PlayerState visual_state{};
        visual_state.health=100.0f;
        PlayerSystem visual(visual_state);
        visual.set_source_skin(skin);
        assert(visual.source_skin()==skin);
        for (const auto facing : {PlayerFacing::down,PlayerFacing::up,
                                  PlayerFacing::right,PlayerFacing::left}) {
            const char* direction="down";
            switch(facing) {
            case PlayerFacing::down: direction="down"; break;
            case PlayerFacing::up: direction="up"; break;
            case PlayerFacing::right: direction="right"; break;
            case PlayerFacing::left: direction="left"; break;
            }
            for (const auto mode : {PlayerAnimation::idle,PlayerAnimation::walk,
                                    PlayerAnimation::run}) {
                char name[24]{};
                std::snprintf(name,sizeof(name),"%s_%s",
                    mode==PlayerAnimation::idle?"idle":"run",direction);
                int expected=-1;
                int index=0;
                for (const auto& original:stage1::kAnimations) {
                    if (original.object!=skin) continue;
                    if (std::strcmp(original.name,name)==0 && original.frames>0)
                        expected=index;
                    ++index;
                }
                assert(expected>=0);
                visual.state_for_setup().facing=facing;
                visual.state_for_setup().animation=mode;
                assert(visual.source_animation_index()==expected);
            }
        }
    }

    // Step 4A: the original per-skin animation table determines both pose
    // index AND the elapsed source-frame coordinate, at each facing.
    // A 20-skin direction test that only checks animation names would miss
    // a wrong frame crop or source-speed mismatch once the character moves.
    for (const std::uint16_t skin : {
             679, 826, 743, 1262, 745, 742, 680, 748, 750, 998,
             997, 825, 811, 749, 1009, 999, 741, 996, 746, 747}) {
        for (const auto face : {PlayerFacing::down,PlayerFacing::up,
                                PlayerFacing::right,PlayerFacing::left}) {
            PlayerState animated_state{};
            animated_state.health=100.0f;
            PlayerSystem original_frames(animated_state);
            original_frames.set_source_skin(skin);
            PlayerInput input{};
            const char* name="run_down";
            switch (face) {
            case PlayerFacing::down: input.move_y=1.0f; name="run_down"; break;
            case PlayerFacing::up: input.move_y=-1.0f; name="run_up"; break;
            case PlayerFacing::right: input.move_x=1.0f; name="run_right"; break;
            case PlayerFacing::left: input.move_x=-1.0f; name="run_left"; break;
            }
            const stage1::AnimationDef* source=nullptr;
            for (const auto& anim : stage1::kAnimations) {
                if (anim.object==skin && std::strcmp(anim.name,name)==0) {
                    source=&anim;break;
                }
            }
            assert(source && source->frames>0 && source->speed>0.0f);
            PlayerRules rules{};
            rules.run_speed=100.0f;
            input.run=true;
            constexpr int kSteps=47;
            constexpr float kDt=0.05f;
            for (int step=0;step<kSteps;++step)
                original_frames.update_movement(input,kDt,rules);
            assert(original_frames.state().facing==face);
            const float elapsed=kSteps*kDt;
            const float cycle=static_cast<float>(source->frames)/source->speed;
            const float effective=source->loop?std::fmod(elapsed,cycle):elapsed;
            const auto advanced=static_cast<std::uint32_t>(effective*source->speed);
            const auto expected=source->loop ? advanced%source->frames :
                                std::min(advanced,source->frames-1);
            assert(original_frames.source_animation_frame()==expected);
            original_frames.update_movement(PlayerInput{},kDt,rules);
            assert(original_frames.source_animation_frame()==0);
            assert(original_frames.state().animation==PlayerAnimation::idle);
        }
    }

    // The original sprite hotspot and rotation determine which source
    // building pixels physically block the player.
    using minidayz::terrain::source_solid_sprite_contains;
    assert(source_solid_sprite_contains(
        100.0f,100.0f,20.0f,10.0f,0.5f,0.5f,0.0f,100.0f,100.0f));
    assert(!source_solid_sprite_contains(
        100.0f,100.0f,20.0f,10.0f,0.5f,0.5f,0.0f,119.0f,100.0f));
    // Rotating 90 degrees about the *hotspot* yields a 10x20 rectangle
    // centred at (100,100), not the formerly displaced top-left box.
    assert(source_solid_sprite_contains(
        100.0f,100.0f,20.0f,10.0f,0.5f,0.5f,90.0f,100.0f,105.0f));
    assert(!source_solid_sprite_contains(
        100.0f,100.0f,20.0f,10.0f,0.5f,0.5f,90.0f,110.0f,105.0f));

    // Non-finite input must not poison player coordinates or stats.
    PlayerInput invalid{};
    invalid.move_x = std::numeric_limits<float>::quiet_NaN();
    edge_player.update_movement(invalid, 0.05f, edge_rules);
    assert(edge_player.state().x == 1.0f);
    assert(std::isfinite(edge_player.state().x));
    assert(std::isfinite(edge_player.state().y));

    // Injected survival rates are applied over a bounded frame. These are
    // API tests, not claims about the original game's tuned gameplay values.
    PlayerState survival_state{};
    survival_state.health = 50.0f;
    survival_state.hunger = 10.0f;
    survival_state.thirst = 10.0f;
    PlayerSystem survival(survival_state);
    PlayerRules survival_rules{};
    survival_rules.max_health = 100.0f;
    survival_rules.max_hunger = 100.0f;
    survival_rules.max_thirst = 100.0f;
    survival_rules.hunger_drain_per_second = 20.0f;
    survival_rules.thirst_drain_per_second = 20.0f;
    survival_rules.regeneration_per_second = 4.0f;
    survival_rules.temperature_change_per_second = 2.0f;
    survival_rules.min_temperature = -10.0f;
    survival_rules.max_temperature = 10.0f;
    const auto survival_result = survival.update_survival(0.25f, survival_rules);
    assert(!survival_result.died);
    assert(survival.state().health == 51.0f);
    assert(survival.state().hunger == 5.0f);
    assert(survival.state().thirst == 5.0f);
    assert(survival.state().temperature == 0.5f);
    assert(survival_result.hunger_lost == 5.0f);
    assert(survival_result.thirst_lost == 5.0f);

    // Unconfigured zero caps must not erase stats while source values
    // are intentionally left unresolved.
    PlayerState unconfigured_state{};
    unconfigured_state.health = 50.0f;
    unconfigured_state.hunger = 37.0f;
    unconfigured_state.thirst = 62.0f;
    PlayerSystem unconfigured(unconfigured_state);
    const auto unchanged = unconfigured.update_survival(0.1f, PlayerRules{});
    assert(!unchanged.died);
    assert(unconfigured.state().hunger == 37.0f);
    assert(unconfigured.state().thirst == 62.0f);

    // Player save/load round-trips all persistent player state and rejects
    // corrupt numeric and enum values without modifying the live player.
    PlayerState persisted_state{};
    persisted_state.x = 12.5f;
    persisted_state.y = -8.0f;
    persisted_state.health = 73.0f;
    persisted_state.hunger = 44.0f;
    persisted_state.thirst = 29.0f;
    persisted_state.temperature = -2.0f;
    persisted_state.bleeding = 1.0f;
    persisted_state.infection = 0.25f;
    persisted_state.facing = PlayerFacing::left;
    persisted_state.animation = PlayerAnimation::run;
    persisted_state.animation_frame = 2;
    persisted_state.animation_elapsed = 2.5f;
    PlayerSystem save_source(persisted_state);
    const PlayerState save_payload = save_source.snapshot();
    PlayerSystem save_target(PlayerState{});
    assert(save_target.restore(save_payload));
    assert(save_target.snapshot().x == 12.5f);
    assert(save_target.snapshot().y == -8.0f);
    assert(save_target.snapshot().health == 73.0f);
    assert(save_target.snapshot().hunger == 44.0f);
    assert(save_target.snapshot().thirst == 29.0f);
    assert(save_target.snapshot().temperature == -2.0f);
    assert(save_target.snapshot().bleeding == 1.0f);
    assert(save_target.snapshot().infection == 0.25f);
    assert(save_target.snapshot().facing == PlayerFacing::left);
    assert(save_target.snapshot().animation == PlayerAnimation::run);
    assert(save_target.snapshot().animation_frame == 2);

    PlayerState corrupt_player = save_payload;
    corrupt_player.health = std::numeric_limits<float>::quiet_NaN();
    assert(!save_target.restore(corrupt_player));
    assert(save_target.state().health == 73.0f);
    corrupt_player = save_payload;
    corrupt_player.facing = static_cast<PlayerFacing>(255);
    assert(!save_target.restore(corrupt_player));
    corrupt_player = save_payload;
    corrupt_player.alive = false;
    assert(!save_target.restore(corrupt_player));

    // Stat setters clamp to the limits supplied by the caller.
    survival.adjust_hunger(200.0f, 100.0f);
    survival.adjust_thirst(-200.0f, 100.0f);
    survival.adjust_temperature(100.0f, -5.0f, 5.0f);
    survival.set_bleeding(3.0f, 2.0f);
    survival.set_infection(-1.0f, 4.0f);
    assert(survival.state().hunger == 100.0f);
    assert(survival.state().thirst == 0.0f);
    assert(survival.state().temperature == 5.0f);
    assert(survival.state().bleeding == 2.0f);
    assert(survival.state().infection == 0.0f);

    // Construct daytime source event assigns the computed cold load,
    // which can be below the preview UI's -10 lower meter bound.
    PlayerState daytime_state{};
    daytime_state.health = 100.0f;
    PlayerSystem daytime(daytime_state);
    daytime.set_source_temperature(-17.0f);
    assert(daytime.state().temperature == -17.0f);
    const float old_day = daytime.state().temperature;
    daytime.set_source_temperature(std::numeric_limits<float>::quiet_NaN());
    assert(daytime.state().temperature == old_day);
    assert(daytime.restore(daytime.snapshot()));
    assert(daytime.state().temperature == -17.0f);
    return 0;
}
