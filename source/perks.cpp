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
    unsigned total = static_cast<unsigned>(candidate.camel_level) +
                     candidate.hamster_level;
    for (const auto other_level : candidate.other_levels) {
        if (other_level > 2) return false;
        total += other_level;
    }
    if (candidate.camel_level > 2 || candidate.hamster_level > 2 ||
        candidate.perks_learnt_count != total ||
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

bool SourcePerkSystem::award_original_xp_event(
    std::uint64_t source_event_uid) noexcept {
    // Original Game_events global expierence_points ADD actions (method 122).
    // Call only on a fully source-verified event dispatch. In particular,
    // don't also call these for Newspaper/Scout Handbook actions already
    // awarded by ItemUseRuntime or award XP every frame.
    std::uint32_t award = 0;
    switch (source_event_uid) {
    case 662365640635533ULL: award = 25; break;
    case 609771835564225ULL: award = 15; break;
    case 184273689853649ULL: award = 200; break;
    case 853523169384096ULL: award = 200; break;
    case 422420671955084ULL: award = 50; break;
    case 278938995521251ULL: award = 50; break;
    case 786944914217829ULL: award = 100; break;
    case 589702583314875ULL: award = 150; break;
    case 979830737090869ULL: award = 100; break;
    case 120764849871488ULL: award = 150; break;
    case 118494092025457ULL: award = 200; break;
    case 920613859152657ULL: award = 250; break;
    case 265440217696674ULL: award = 300; break;
    case 1074032691412307ULL: award = 2; break;
    case 3302144065259174ULL: award = 2; break;
    case 409652067374260ULL: award = 10; break;
    case 517726037510695ULL: award = 5; break;
    case 972069878318185ULL: award = 5; break;
    default: return false;
    }
    return award_experience(award);
}

std::uint8_t SourcePerkSystem::level(SourcePerk perk) const noexcept {
    switch (perk) {
    case SourcePerk::camel: return state_.camel_level;
    case SourcePerk::hamster: return state_.hamster_level;
    default:
        const auto index = static_cast<std::size_t>(perk);
        return index >= 2 && index < kOriginalPerkCount
            ? state_.other_levels[index - 2] : 0;
    }
}

bool SourcePerkSystem::can_buy(SourcePerk perk) const noexcept {
    // Never advertise unknown enum values as purchasable. The source has
    // exactly sixteen purchasable perks (indices 0..15).
    if (static_cast<std::size_t>(perk) >= kOriginalPerkCount) return false;
    return level(perk) < 2 && state_.experience_points >= state_.perk_cost &&
        state_.perks_learnt_count < 2 * kOriginalPerkCount;
}

bool SourcePerkSystem::buy(SourcePerk perk) noexcept {
    if (!can_buy(perk)) return false;
    SourcePerkSnapshot next = state_;
    switch (perk) {
    case SourcePerk::camel: ++next.camel_level; break;
    case SourcePerk::hamster: ++next.hamster_level; break;
    default: {
        const auto index = static_cast<std::size_t>(perk);
        if (index < 2 || index >= kOriginalPerkCount) return false;
        ++next.other_levels[index - 2];
        break;
    }
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
