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
} // namespace minidayz::items
