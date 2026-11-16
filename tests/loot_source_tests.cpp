#include "minidayz/loot_source.hpp"
#include "minidayz/inventory.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
using namespace minidayz::items;

int main() {
    assert(kSourceLootPoolCount > 0);
    assert(kSourceLootChoiceCount >= kSourceLootPoolCount);
    std::uint64_t previous = 0;
    for (std::size_t i = 0; i < kSourceLootPoolCount; ++i) {
        const auto& pool = kSourceLootPools[i];
        assert(pool.action_uid >= previous && pool.choices > 0);
        previous = pool.action_uid;
        assert(pool.first + pool.choices <= kSourceLootChoiceCount);
        assert(source_loot_pool(i) == &pool);
        for (std::size_t choice = 0; choice < pool.choices; ++choice) {
            const auto result = source_loot_choice(i, choice);
            assert(source_item_exists(result));
            assert(result == kSourceLootChoices[pool.first + choice]);
        }
        assert(source_loot_choice(i, pool.choices) == 0);
        std::uint16_t selected = 0;
        assert(source_loot_choose_from_draw(i, 0, selected));
        assert(selected == source_loot_choice(i, 0));
        assert(source_loot_choose_from_draw(i, UINT32_MAX, selected));
        assert(selected == source_loot_choice(i, pool.choices - 1));
        // Every possible choice bucket is reachable from the 32-bit domain.
        for (std::size_t choice = 0; choice < pool.choices; ++choice) {
            const auto draw = static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(choice) << 32) / pool.choices + 1);
            assert(source_loot_choose_from_draw(i, draw, selected));
            assert(selected == source_loot_choice(i, choice));
        }
    }
    assert(source_loot_pool(kSourceLootPoolCount) == nullptr);
    assert(source_loot_choice(kSourceLootPoolCount, 0) == 0);
    std::uint16_t untouched = 65535;
    assert(!source_loot_choose_from_draw(kSourceLootPoolCount, 0, untouched));
    assert(untouched == 65535);
    return 0;
}
