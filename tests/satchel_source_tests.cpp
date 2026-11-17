#include "minidayz/satchel_source.hpp"
#include <cassert>
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
    return 0;
}
