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
        // Raw localization text IDs are not item IDs: do not infer a mapping.
        if (i != 0)
            assert(kSourceItemDescriptions[i-1].item_id < row.item_id);
    }
    assert(kVerifiedItemDescriptionCount > 100);
    for (std::size_t i = 0; i < kVerifiedItemDescriptionCount; ++i) {
        const auto& row = kVerifiedItemDescriptions[i];
        assert(row.description);
        assert(source_item_description(row.item_id) == row.description);
        if (i > 0) assert(kVerifiedItemDescriptions[i-1].item_id < row.item_id);
        bool matching_raw = false;
        for (std::size_t j = 0; j < kSourceItemDescriptionCount; ++j) {
            if (kSourceItemDescriptions[j].item_id == row.source_text_id) {
                assert(kSourceItemDescriptions[j].description ==
                       row.description || std::strcmp(
                           kSourceItemDescriptions[j].description,
                           row.description) == 0);
                matching_raw = true;
                break;
            }
        }
        assert(matching_raw);
    }
    // These exact mismatches exist in the original root XML. Previously
    // treating the description ID as item ID showed NPC dialogue for a bow
    // and a gas-mask description for the Improvised Sling Bag.
    const char* bow = source_item_description(163);
    const char* bag = source_item_description(454);
    const char* tent = source_item_description(456);
    assert(bow && std::strstr(bow, "functional bow"));
    assert(bag && std::strstr(bag, "burlap bag"));
    assert(tent && std::strstr(tent, "civilian camping tent"));
    const char* scope=source_item_description(124);
    assert(scope && std::strstr(scope,"red laser beam"));
    const char* fried_eggs=source_item_description(724);
    assert(fried_eggs && std::strstr(fried_eggs,"fried egg"));
    for (std::uint16_t id=761;id<=776;++id) {
        const char* box=source_item_description(id);
        assert(box && std::strstr(box,"10 times"));
    }
    for (std::uint16_t id = 0; id <= 900; ++id) {
        const char* desc = source_item_description(id);
        if (desc) assert(std::strlen(desc) < 4096);
    }
    return 0;
}
