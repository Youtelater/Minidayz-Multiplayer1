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
        assert(pool.action_uid > previous && pool.choices > 0);
        previous = pool.action_uid;
        assert(pool.first + pool.choices <= kSourceLootChoiceCount);
        assert(source_loot_pool(pool.action_uid) == &pool);
        for (std::size_t choice = 0; choice < pool.choices; ++choice) {
            const auto result = source_loot_choice(pool.action_uid, choice);
            assert(source_item_exists(result));
            assert(result == kSourceLootChoices[pool.first + choice]);
        }
        assert(source_loot_choice(pool.action_uid, pool.choices) == 0);
    }
    assert(source_loot_pool(0) == nullptr);
    assert(source_loot_choice(0, 0) == 0);
    return 0;
}
