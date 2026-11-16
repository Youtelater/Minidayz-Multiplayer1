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
