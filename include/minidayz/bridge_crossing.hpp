#pragma once

#include "minidayz/player.hpp"

namespace minidayz {

// The crossing uses local coordinates so an interrupted trip can resume
// from the last island save without writing a bridge position into it.
class BridgeCrossing {
public:
    static constexpr float kLength=720.0f;
    static constexpr float kCenterY=120.0f;
    static constexpr float kHalfWidth=46.0f;
    static constexpr float kExitStart=kLength-36.0f;

    void enter(const PlayerSystem& survivor) noexcept {
        walker_=survivor;
        auto& position=walker_.state_for_setup();
        position.x=20.0f;
        position.y=kCenterY;
        position.facing=PlayerFacing::right;
        position.animation=PlayerAnimation::idle;
        position.animation_frame=0;
        position.animation_elapsed=0.0f;
        active_=true;
    }

    void leave() noexcept { active_=false; }
    bool active() const noexcept { return active_; }
    bool at_exit() const noexcept {
        return active_ && walker_.state().x>=kExitStart;
    }
    const PlayerSystem& walker() const noexcept { return walker_; }

    void move(const PlayerInput& input,float dt,const PlayerRules& rules) noexcept {
        if (active_) walker_.update_movement(input,dt,rules,&blocked);
    }

private:
    static bool blocked(void*,float x,float y) noexcept {
        constexpr float edge_margin=8.0f;
        return x<edge_margin || x>kLength-edge_margin ||
               y<kCenterY-kHalfWidth+edge_margin ||
               y>kCenterY+kHalfWidth-edge_margin;
    }

    PlayerSystem walker_{{}};
    bool active_{};
};

} // namespace minidayz
