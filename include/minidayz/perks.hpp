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
