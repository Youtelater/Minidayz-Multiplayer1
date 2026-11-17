#include "minidayz/satchel_source.hpp"
#include <cassert>
#include <initializer_list>
using namespace minidayz::items;
int main() {
    for (auto id : {458u, 752u}) {
        const auto slots = source_satchel_capacity(static_cast<std::uint16_t>(id));
        assert(slots.alongside_backpack && slots.unrestricted == 2 &&
               slots.exclusive == 0);
    }
    for (auto id : {459u, 460u, 753u, 754u}) {
        const auto slots = source_satchel_capacity(static_cast<std::uint16_t>(id));
        assert(slots.alongside_backpack && slots.unrestricted == 1 &&
               slots.exclusive == 3);
    }
    for (auto id : {0u, 450u, 451u, 461u, 462u}) {
        const auto slots = source_satchel_capacity(static_cast<std::uint16_t>(id));
        assert(!slots.alongside_backpack && slots.unrestricted == 0 &&
               slots.exclusive == 0);
    }
    // Exact Construct player-stats exclusion guards (not category guesses).
    for (auto item : {24,42,43,44,20,55,125}) {
        assert(source_satchel_exclusive_accepts(459, item));
        assert(source_satchel_exclusive_accepts(753, item));
        assert(!source_satchel_exclusive_accepts(460, item));
    }
    for (auto item : {18,19,28,29,45,46,109,118,708,710,143,759,760}) {
        assert(source_satchel_exclusive_accepts(460, item));
        assert(source_satchel_exclusive_accepts(754, item));
        assert(!source_satchel_exclusive_accepts(459, item));
    }
    assert(source_satchel_exclusive_accepts(459, 0));
    assert(source_satchel_exclusive_accepts(460, 0));
    assert(!source_satchel_exclusive_accepts(459, 99));
    assert(!source_satchel_exclusive_accepts(460, 99));
    assert(!source_satchel_exclusive_accepts(458, 18));
    assert(source_supplies_stash_slots(461) == 6);
    assert(source_supplies_stash_slots(456) == 0);
    assert(source_supplies_stash_slots(460) == 0);
    return 0;
}
