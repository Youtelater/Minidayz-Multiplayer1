#pragma once
#include "minidayz/generated/sprite_catalog.hpp"
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
    const SpriteAnimationDef& animation,float elapsed_seconds) noexcept {
    if (!animation.frame_count || !std::isfinite(elapsed_seconds) ||
        !std::isfinite(animation.source_speed) ||
        animation.source_speed <= 0.0f)
        return 0;
    const double elapsed=std::max(0.0,static_cast<double>(elapsed_seconds));
    const double cycle=static_cast<double>(animation.frame_count) /
                       static_cast<double>(animation.source_speed);
    const double seconds=animation.loop && cycle>0.0 ?
        std::fmod(elapsed,cycle) : elapsed;
    const double frames=std::floor(seconds *
                       static_cast<double>(animation.source_speed));
    // Constrain before narrowing: extremely long sessions must not overflow.
    if (frames>=static_cast<double>(animation.frame_count))
        return animation.frame_count-1;
    return frames>0.0 ? static_cast<std::uint32_t>(frames) : 0u;
}

inline std::uint32_t source_equipment_animation_frame(
    std::uint16_t object,const char* animation_name,
    float elapsed_seconds) noexcept {
    if (!animation_name || !animation_name[0]) return 0;
    const auto* begin=kSpriteAnimations;
    const auto* end=begin+kSpriteAnimationCount;
    const auto* it=std::lower_bound(begin,end,object,
        [](const SpriteAnimationDef& a,std::uint16_t wanted) {
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
