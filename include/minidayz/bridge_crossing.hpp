#pragma once

#include "minidayz/player.hpp"
#include <cstddef>

namespace minidayz {

// The crossing uses local coordinates so an interrupted trip can resume
// from the last island save without writing a bridge position into it.
enum class Passage : unsigned char { bridge, bunker, final_stand };
struct BridgePosition { float x{},y{}; Passage area{Passage::bridge}; };

class BridgeCrossing {
public:
    static constexpr float kLength=720.0f;
    static constexpr float kCenterY=120.0f;
    static constexpr float kHalfWidth=46.0f;
    static constexpr float kExitStart=kLength-36.0f;

    void enter(const PlayerSystem& survivor, Passage area=Passage::bridge) noexcept {
        area_=area;
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

    static bool valid_position(BridgePosition p) noexcept {
        return (p.area==Passage::bridge || p.area==Passage::bunker || p.area==Passage::final_stand) &&
            !obstructed(p.area,p.x,p.y) && p.x>=0 && p.x<=kLength && p.y>=kCenterY-kHalfWidth && p.y<=kCenterY+kHalfWidth;
    }
    BridgePosition position() const noexcept {
        return {walker_.state().x,walker_.state().y,area_};
    }
    void resume(const PlayerSystem& survivor,BridgePosition p) noexcept {
        active_=false;
        if (!valid_position(p)) return;
        enter(survivor,p.area);
        walker_.state_for_setup().x=p.x;
        walker_.state_for_setup().y=p.y;
    }
    void leave() noexcept { active_=false; }
    Passage area() const noexcept { return area_; }
    bool active() const noexcept { return active_; }
    bool at_exit() const noexcept {
        return active_ && walker_.state().x>=kExitStart;
    }
    const PlayerSystem& walker() const noexcept { return walker_; }

    void move(const PlayerInput& input,float dt,const PlayerRules& rules) noexcept {
        if (active_) walker_.update_movement(input,dt,rules,&blocked,this);
    }

    struct Partition { float x,top,bottom; };
    static constexpr Partition partitions[]{{225,74,125},{455,115,166}};
    static bool has_partition(Passage area,std::size_t i) noexcept {
        return area==Passage::final_stand || (area==Passage::bunker && i==0);
    }

private:
    static bool obstructed(Passage area,float x,float y) noexcept {
        for (std::size_t i=0;i<2;++i) {
            const auto& wall=partitions[i];
            if (has_partition(area,i) && x>=wall.x && x<=wall.x+24 &&
                y>=wall.top && y<=wall.bottom) return true;
        }
        return false;
    }
    static bool blocked(void* context,float x,float y) noexcept {
        constexpr float edge_margin=8.0f;
        return obstructed(static_cast<BridgeCrossing*>(context)->area_,x,y) || x<edge_margin || x>kLength-edge_margin ||
               y<kCenterY-kHalfWidth+edge_margin ||
               y>kCenterY+kHalfWidth-edge_margin;
    }

    PlayerSystem walker_{{}};
    Passage area_{Passage::bridge};
    bool active_{};
};

} // namespace minidayz
