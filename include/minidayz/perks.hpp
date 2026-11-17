#pragma once
#include "minidayz/item_use.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// Original Game_events globals: perk_hamster, perk_camel,
// expierence_points, perk_cost and perks_learnt_count.
// Both item bonuses are applied only at the source's level == 2.
// All sixteen original Perks and stats menu variables. Source menu indices
// 3=camel, 4=hamster, 7=regen are verified against the Construct click guards.
enum class SourcePerk : std::uint8_t {
    camel, hamster, header, spotter, runner, snowborn, farmer, regen,
    hunter, meleeatk, meleedef, trunkster, sweethome, metabolism, eye, agronom
};
inline constexpr std::size_t kOriginalPerkCount = 16;
inline constexpr std::size_t kOtherPerkCount = kOriginalPerkCount - 2;
inline constexpr SourcePerk kOriginalPerkMenuOrder[kOriginalPerkCount] = {
    SourcePerk::header, SourcePerk::spotter, SourcePerk::runner,
    SourcePerk::camel, SourcePerk::hamster, SourcePerk::snowborn,
    SourcePerk::farmer, SourcePerk::regen, SourcePerk::meleedef,
    SourcePerk::meleeatk, SourcePerk::trunkster, SourcePerk::hunter,
    SourcePerk::sweethome, SourcePerk::metabolism, SourcePerk::eye,
    SourcePerk::agronom
};
inline constexpr const char* kOriginalPerkMenuLabels[kOriginalPerkCount] = {
    "HEADER", "SPOTTER", "RUNNER", "CAMEL", "HAMSTER", "SNOWBORN",
    "FARMER", "REGEN", "MELEE DEF", "MELEE ATK", "TRUNKSTER",
    "HUNTER", "SWEET HOME", "METABOLISM", "EYE", "AGRONOM"
};

struct SourcePerkSnapshot {
    std::uint32_t experience_points{};
    std::uint32_t perk_cost{100};
    std::uint16_t perks_learnt_count{};
    std::uint8_t camel_level{};
    std::uint8_t hamster_level{};
    // The original fourteen other perk variables use the same 0/1/2 levels.
    // Stored after the legacy v8 fields so v8 save migration stays exact.
    std::array<std::uint8_t, kOtherPerkCount> other_levels{};
};

class SourcePerkSystem {
public:
    const SourcePerkSnapshot& snapshot() const noexcept { return state_; }
    bool restore(const SourcePerkSnapshot& candidate) noexcept;
    // A reset must retain the original first-purchase threshold of 100 XP.
    void clear() noexcept { state_ = SourcePerkSnapshot{}; }
    // Experience is awarded only by a translated original XP event;
    // never award arbitrary XP from elapsed time.
    bool award_experience(std::uint32_t source_award) noexcept;
    // Call only after a matching original Construct event guard fires.
    // Unknown or context-dependent XP actions are rejected, not guessed.
    bool award_original_xp_event(std::uint64_t source_event_uid) noexcept;
    bool can_buy(SourcePerk perk) const noexcept;
    bool buy(SourcePerk perk) noexcept;
    std::uint8_t level(SourcePerk perk) const noexcept;
    ItemUsePerks item_use_flags() const noexcept {
        return {state_.hamster_level == 2, state_.camel_level == 2};
    }
    // The following are exact original conditional event outputs, not
    // unconditional combat/bleeding timers. The caller must satisfy the
    // original attack, wound or storage event before applying them.
    // Melee defense events 692233337832379 / 417286342579831
    // ADD 5/15 to global def_percent for levels 1/2.
    std::uint8_t source_melee_defense_percent_bonus() const noexcept {
        const auto acquired = level(SourcePerk::meleedef);
        return acquired >= 2 ? 15 : acquired == 1 ? 5 : 0;
    }
    // Crit_bonus global SET 10 at event 8638551322453103,
    // if original perk_meleeatk > 0.
    std::uint8_t source_melee_critical_bonus() const noexcept {
        return level(SourcePerk::meleeatk) > 0 ? 10 : 0;
    }
    // Trunk_bonus global SET 10 at events 792853637969588 and
    // 960702164997405 when source perk_trunkster > 0.
    std::uint8_t source_trunk_bonus() const noexcept {
        return level(SourcePerk::trunkster) > 0 ? 10 : 0;
    }
    // Wound timers from event 791147402355693 / 507447921773060:
    // ternary perk_metabolism == 2 ? 30 seconds : 60 seconds.
    std::uint8_t source_bleeding_timer_seconds() const noexcept {
        return level(SourcePerk::metabolism) == 2 ? 30 : 60;
    }
    // Sweethome event 376072841791098 adds 0.15 to each of hunger
    // and thirst in its guarded player-home context, only above level 1.
    float source_sweethome_needs_gain() const noexcept {
        return level(SourcePerk::sweethome) >= 2 ? 0.15f : 0.0f;
    }

private:
    SourcePerkSnapshot state_{};
};

} // namespace minidayz::items
