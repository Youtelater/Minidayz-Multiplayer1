#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace minidayz::stage1 {

// Root Game_events / Interaction / Using inventory items:
// UID 435941792839578, item_activate=63 starts PlayerCollision181's
// Timer "Edrink" at 60 seconds and sets PlayerBase193 variable 4 to +15.
// Player stats UID 197231340522492 resets variable 4 to zero when that
// timer expires. The timer belongs to the survivor, not the inventory icon.
class SourceEnergyDrinkBoost {
public:
    static constexpr std::uint16_t kSourceItemId=63;
    static constexpr float kSourceDurationSeconds=60.0f;
    static constexpr float kSourceSpeedBonus=15.0f;

    bool on_successful_item_use(std::uint16_t item_id,
                                bool secondary,bool consumed) noexcept {
        if(!consumed || secondary || item_id!=kSourceItemId)return false;
        seconds_remaining_=kSourceDurationSeconds; // Timer.Start restarts it.
        return true;
    }
    void update(float seconds) noexcept {
        if(!std::isfinite(seconds)||seconds<=0.0f)return;
        seconds_remaining_=std::max(0.0f,seconds_remaining_-seconds);
    }
    float bonus() const noexcept {
        return seconds_remaining_>0.0f?kSourceSpeedBonus:0.0f;
    }
    float remaining() const noexcept {return seconds_remaining_;}
    static bool valid(float seconds) noexcept {
        return std::isfinite(seconds) && seconds>=0.0f &&
               seconds<=kSourceDurationSeconds;
    }
    bool restore(float seconds) noexcept {
        if(!valid(seconds))return false;
        seconds_remaining_=seconds;
        return true;
    }
    void clear() noexcept {seconds_remaining_=0.0f;}
private:
    float seconds_remaining_{};
};

} // namespace minidayz::stage1
