#pragma once

#include "minidayz/fractional_wear.hpp"
#include "minidayz/generated/source_wear_catalog.hpp"
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// This is an ACTION dispatcher, not an event-condition interpreter. The
// original nested Construct group/ancestor/event guards must all have
// succeeded before callers pass source_guards_satisfied=true. No item-name
// approximations, per-frame wear or random/untranslated actions are applied.
inline const SourceWearAction* source_wear_action(
    std::uint64_t group_uid,
    std::uint64_t event_uid,
    std::uint64_t action_uid) noexcept {
    for (std::size_t i=0; i<kSourceWearActionCount; ++i) {
        const auto& action=kSourceWearActions[i];
        if (action.group_uid==group_uid &&
            action.event_uid==event_uid &&
            action.action_uid==action_uid)
            return &action;
    }
    return nullptr;
}

// Uses fractional hundredths, so original 0.15/0.25/0.5 etc are not rounded
// away. perk_wxnj level one suppresses wear exactly where the source action
// explicitly has the perk_wxnj ternary; elsewhere it is never presumed.
inline bool apply_verified_source_wear(
    std::uint64_t group_uid,
    std::uint64_t event_uid,
    std::uint64_t action_uid,
    bool source_guards_satisfied,
    bool perk_wxnj_active,
    SourceFractionalWear& condition) noexcept {
    if (!source_guards_satisfied) return false;
    const auto* source=source_wear_action(
        group_uid,event_uid,action_uid);
    if (!source) return false;
    const std::uint16_t amount=
        source->cancel_when_wxnj_active && perk_wxnj_active
            ? 0 : source->loss_hundredths;
    return amount!=0 && condition.wear(amount);
}

} // namespace minidayz::items
