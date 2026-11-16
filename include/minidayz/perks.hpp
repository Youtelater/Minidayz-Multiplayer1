#pragma once
#include "minidayz/item_use.hpp"
#include <cstdint>

namespace minidayz::items {

// Original Game_events globals: perk_hamster, perk_camel,
// expierence_points, perk_cost and perks_learnt_count.
// Both item bonuses are applied only at the source's level == 2.
enum class SourcePerk : std::uint8_t { camel, hamster };

struct SourcePerkSnapshot {
    std::uint32_t experience_points{};
    std::uint32_t perk_cost{100};
    std::uint16_t perks_learnt_count{};
    std::uint8_t camel_level{};
    std::uint8_t hamster_level{};
};

class SourcePerkSystem {
public:
    const SourcePerkSnapshot& snapshot() const noexcept { return state_; }
    bool restore(const SourcePerkSnapshot& candidate) noexcept;
    void clear() noexcept { state_ = {}; }
    // Experience is awarded only by a translated original XP event;
    // never award arbitrary XP from elapsed time.
    bool award_experience(std::uint32_t source_award) noexcept;
    bool can_buy(SourcePerk perk) const noexcept;
    bool buy(SourcePerk perk) noexcept;
    std::uint8_t level(SourcePerk perk) const noexcept;
    ItemUsePerks item_use_flags() const noexcept {
        return {state_.hamster_level == 2, state_.camel_level == 2};
    }
private:
    SourcePerkSnapshot state_{};
};

} // namespace minidayz::items
