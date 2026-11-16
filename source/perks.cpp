#include "minidayz/perks.hpp"

#include <limits>

namespace minidayz::items {
namespace {
constexpr std::uint32_t kOriginalFirstCost = 100u;
constexpr std::uint32_t kOriginalBaseCostIncrement = 100u;
constexpr std::uint32_t kOriginalPerPurchaseIncrement = 20u;

// Original purchase event UID 454712397231976:
// perk_cost = perk_cost + 100 + (20 * perks_learnt_count).
bool correct_source_cost(const SourcePerkSnapshot& state) noexcept {
    std::uint64_t expected = kOriginalFirstCost;
    for (std::uint32_t i = 1; i <= state.perks_learnt_count; ++i) {
        expected += kOriginalBaseCostIncrement +
                    kOriginalPerPurchaseIncrement * i;
        if (expected > std::numeric_limits<std::uint32_t>::max()) return false;
    }
    return state.perk_cost == expected;
}
}

bool SourcePerkSystem::restore(const SourcePerkSnapshot& candidate) noexcept {
    if (candidate.camel_level > 2 || candidate.hamster_level > 2 ||
        candidate.perks_learnt_count !=
            static_cast<unsigned>(candidate.camel_level) + candidate.hamster_level ||
        !correct_source_cost(candidate))
        return false;
    state_ = candidate;
    return true;
}

bool SourcePerkSystem::award_experience(std::uint32_t source_award) noexcept {
    if (source_award == 0 ||
        source_award > std::numeric_limits<std::uint32_t>::max() -
            state_.experience_points)
        return false;
    state_.experience_points += source_award;
    return true;
}

std::uint8_t SourcePerkSystem::level(SourcePerk perk) const noexcept {
    switch (perk) {
    case SourcePerk::camel: return state_.camel_level;
    case SourcePerk::hamster: return state_.hamster_level;
    }
    return 0;
}

bool SourcePerkSystem::can_buy(SourcePerk perk) const noexcept {
    return level(perk) < 2 && state_.experience_points >= state_.perk_cost &&
        state_.perks_learnt_count < 4;
}

bool SourcePerkSystem::buy(SourcePerk perk) noexcept {
    if (!can_buy(perk)) return false;
    SourcePerkSnapshot next = state_;
    switch (perk) {
    case SourcePerk::camel: ++next.camel_level; break;
    case SourcePerk::hamster: ++next.hamster_level; break;
    default: return false;
    }
    // The original Perks and stats menu tests cumulative experience against
    // perk_cost. It raises perk_cost on purchase but does not subtract XP.
    ++next.perks_learnt_count;
    const std::uint64_t new_cost = static_cast<std::uint64_t>(next.perk_cost) +
        kOriginalBaseCostIncrement +
        kOriginalPerPurchaseIncrement * next.perks_learnt_count;
    if (new_cost > std::numeric_limits<std::uint32_t>::max()) return false;
    next.perk_cost = static_cast<std::uint32_t>(new_cost);
    if (!correct_source_cost(next)) return false;
    state_ = next;
    return true;
}
} // namespace minidayz::items
