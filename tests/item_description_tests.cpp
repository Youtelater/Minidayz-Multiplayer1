#include "minidayz/item_description.hpp"
#include "minidayz/generated/item_catalog.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
using namespace minidayz::items;
int main() {
    assert(kSourceItemDescriptionCount == 528);
    assert(kItemsWithoutSourceDescription == 20);
    for (std::size_t i = 0; i < kSourceItemDescriptionCount; ++i) {
        const auto& row = kSourceItemDescriptions[i];
        assert(row.description);
        assert(source_item_description(row.item_id) == row.description);
        if (i != 0)
            assert(kSourceItemDescriptions[i-1].item_id < row.item_id);
    }
    for (std::uint16_t id = 0; id <= 900; ++id) {
        const char* desc = source_item_description(id);
        if (desc) assert(std::strlen(desc) < 4096);
    }
    return 0;
}
