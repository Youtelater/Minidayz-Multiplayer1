#pragma once
#include "minidayz/generated/loot_source_pools.hpp"
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// Original Construct's choose(...) arguments are kept in their exact order,
// including duplicates. Caller provides the already evaluated choice index
// after matching the original event conditions. No invented respawn cadence,
// source-item weights, or RNG algorithm is supplied here.
inline const SourceLootPool* source_loot_pool(std::uint64_t action_uid) noexcept {
    std::size_t first = 0;
    std::size_t last = kSourceLootPoolCount;
    while (first < last) {
        const std::size_t mid = first + (last - first) / 2;
        if (kSourceLootPools[mid].action_uid < action_uid) first = mid + 1;
        else last = mid;
    }
    return first < kSourceLootPoolCount &&
           kSourceLootPools[first].action_uid == action_uid
        ? &kSourceLootPools[first] : nullptr;
}

inline std::uint16_t source_loot_choice(std::uint64_t action_uid,
                                         std::size_t choice_index) noexcept {
    const auto* pool = source_loot_pool(action_uid);
    if (!pool || choice_index >= pool->choices) return 0;
    return kSourceLootChoices[pool->first + choice_index];
}
} // namespace minidayz::items
