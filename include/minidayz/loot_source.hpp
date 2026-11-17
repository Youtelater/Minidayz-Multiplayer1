#pragma once
#include "minidayz/generated/loot_source_pools.hpp"
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// The original Construct export sometimes clones ACE action UIDs in different
// branches. We therefore address an exact source action occurrence by table
// index, retaining both original action and event UIDs for traceability.
// Never collapse two source branches based solely on their shared UID.
// An original matching-condition and RNG evaluator supplies choice_index.
inline const SourceLootPool* source_loot_pool(std::size_t record_index) noexcept {
    return record_index < kSourceLootPoolCount
        ? &kSourceLootPools[record_index] : nullptr;
}

// A source event can spawn a drop only after ALL original Construct
// ancestry has been evaluated. Literal marker (object 218 variable 0)
// and SEED_pubg comparisons are extracted directly from the same event
// tree as the choice pool; unsupported conditions are explicitly caller
// validated, never assumed true by a timer or random choice.
struct SourceLootGuardContext {
    std::int16_t marker{};
    std::int8_t seed_pubg{};
    bool original_respawn_event_fired{};
    bool all_remaining_original_guards_satisfied{};
};

inline bool source_loot_dispatch_guarded(
    std::size_t record_index,
    const SourceLootGuardContext& guards) noexcept {
    const auto* pool = source_loot_pool(record_index);
    if (!pool || !guards.original_respawn_event_fired ||
        !guards.all_remaining_original_guards_satisfied)
        return false;
    return (pool->marker < 0 || guards.marker == pool->marker) &&
           (pool->seed_pubg < 0 || guards.seed_pubg == pool->seed_pubg);
}

inline std::uint16_t source_loot_choice(std::size_t record_index,
                                         std::size_t choice_index) noexcept {
    const auto* pool = source_loot_pool(record_index);
    if (!pool || choice_index >= pool->choices) return 0;
    return kSourceLootChoices[pool->first + choice_index];
}

// Resolve a Construct choose(...) selection from a caller-supplied uniform
// unsigned draw. The modulo is intentionally not used: it biases choices for
// pool sizes that do not divide 2^32. This helper never decides when/where to
// spawn loot, which still depends on the original event conditions.
inline bool source_loot_choose_from_draw(std::size_t record_index,
                                         std::uint32_t draw,
                                         std::uint16_t& item_id) noexcept {
    const auto* pool = source_loot_pool(record_index);
    if (!pool || pool->choices == 0) return false;
    const std::uint64_t scaled =
        (static_cast<std::uint64_t>(draw) * pool->choices) >> 32;
    item_id = kSourceLootChoices[pool->first +
        static_cast<std::size_t>(scaled)];
    return true;
}

} // namespace minidayz::items
