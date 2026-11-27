#pragma once
#include "minidayz/generated/stage1_data.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <strings.h>

namespace minidayz::stage1 {

// Every original Construct Sprite animation has its own frame count, playback
// speed and loop flag. Equipping a coat or holding a weapon does NOT turn its
// separate animation into the selected skin's frame index. Reuse the same
// elapsed movement phase, but interpret it using that original Sprite's
// source-authored animation metadata, including nonlooping animations.
inline std::uint32_t source_animation_frame_at_time(
    const AnimationDef& animation,float elapsed_seconds) noexcept {
    if (!animation.frames || !std::isfinite(elapsed_seconds) ||
        !std::isfinite(animation.speed) ||
        animation.speed <= 0.0f)
        return 0;
    const double elapsed=std::max(0.0,static_cast<double>(elapsed_seconds));
    const double elapsed_frames=std::floor(elapsed *
                                           static_cast<double>(animation.speed));
    const double frames=animation.loop
        ? std::fmod(elapsed_frames,static_cast<double>(animation.frames))
        : elapsed_frames;
    // Constrain before narrowing: extremely long sessions must not overflow.
    if (frames>=static_cast<double>(animation.frames))
        return animation.frames-1;
    return frames>0.0 ? static_cast<std::uint32_t>(frames) : 0u;
}

// Non-player Sprite systems (e.g. the six original NPC bodies) store
// Construct's 0-based animation INDEX, not its name. Resolve the source
// entry for that exact object instead of guessing 8 FPS/4 frames for
// every body. kAnimations is generated in original object/animation order.
inline std::uint32_t source_object_animation_frame(
    std::uint16_t object,std::uint16_t animation_index,
    float elapsed_seconds) noexcept {
    const auto* end=kAnimations+kAnimationCount;
    const auto* found=std::lower_bound(kAnimations,end,object,
        [](const AnimationDef& a,std::uint16_t id){
            return a.object<id;
        });
    for(std::uint16_t i=0;i<animation_index &&
        found!=end && found->object==object;++i)
        ++found;
    if(found==end || found->object!=object)return 0;
    return source_animation_frame_at_time(*found,elapsed_seconds);
}

inline std::uint32_t source_equipment_animation_frame(
    std::uint16_t object,const char* animation_name,
    float elapsed_seconds) noexcept {
    if (!animation_name || !animation_name[0]) return 0;
    const auto* begin=kAnimations;
    const auto* end=begin+kAnimationCount;
    const auto* it=std::lower_bound(begin,end,object,
        [](const AnimationDef& a,std::uint16_t wanted) {
            return a.object<wanted;
        });
    for (;it!=end && it->object==object;++it)
        if (::strcasecmp(it->name,animation_name)==0)
            return source_animation_frame_at_time(*it,elapsed_seconds);
    // A missing pose is a source-coverage failure, NOT permission to invent
    // a substitute animation or a frame index from another Sprite.
    return 0;
}

} // namespace minidayz::stage1
