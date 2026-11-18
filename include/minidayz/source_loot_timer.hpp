#pragma once
#include <cmath>
#include <cstdint>

namespace minidayz::items {

// Original root Game_events Start events UID 8520771380755414:
// Timer.Start(LOOT_RESPAWN_TIME, repeat=1, "Check_loot").
// Original root initial LOOT_RESPAWN_TIME is 1480 seconds.
// Check_loot event UID 2026232979865106 calls Global_respawn,
// whose remaining world/entity conditions must be checked separately.
struct SourceLootTimerSnapshot {
    float elapsed_seconds{};
    std::uint32_t firings{};
};
class SourceLootTimer {
public:
    static constexpr float kOriginalRespawnSeconds=1480.0f;
    static constexpr std::uint64_t kSourceCheckLootEvent=2026232979865106ULL;
    const SourceLootTimerSnapshot& snapshot() const noexcept {return state_;}
    void reset() noexcept {state_={};}
    bool restore(const SourceLootTimerSnapshot& s) noexcept {
        if (!std::isfinite(s.elapsed_seconds) || s.elapsed_seconds<0.0f ||
            s.elapsed_seconds>=kOriginalRespawnSeconds) return false;
        state_=s;
        return true;
    }
    // Returns true only when a source Check_loot timer event actually fires.
    // A caller must evaluate original Map/bridge/marker conditions before
    // dispatching even one item or inferring Global_respawn effects.
    bool advance(float dt) noexcept {
        if (!std::isfinite(dt) || dt<0.0f || dt>1.0f) return false;
        state_.elapsed_seconds+=dt;
        if (state_.elapsed_seconds<kOriginalRespawnSeconds) return false;
        state_.elapsed_seconds-=kOriginalRespawnSeconds;
        ++state_.firings;
        return true;
    }
private:
    SourceLootTimerSnapshot state_{};
};
} // namespace minidayz::items
