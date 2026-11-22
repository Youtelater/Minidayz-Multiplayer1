#include "minidayz/generated/terrain_scatter_source.hpp"
#include <cassert>
#include <cstdint>
#include <set>

int main() {
    using namespace minidayz::stage1;
    assert(kTerrainScatterRuleCount >= 45);
    assert(kTerrainScatterChoiceCount >= kTerrainScatterRuleCount);
    std::set<std::uint16_t> markers;
    for (const auto& rule : kTerrainScatterRules) {
        assert(rule.repeat > 0 && rule.repeat <= 400);
        assert(rule.x_min < rule.x_max && rule.x_max <= 33);
        assert(rule.y_min < rule.y_max && rule.y_max <= 33);
        assert(rule.tile_count > 0);
        assert(rule.tile_offset + rule.tile_count <= kTerrainScatterChoiceCount);
        assert(rule.source_action_uid != 0);
        assert(rule.level_mask != 0 && (rule.level_mask & ~0x3fu) == 0);
        markers.insert(rule.marker);
        for (std::uint32_t n=0;n<rule.tile_count;++n) {
            // All source texture choices reference object 230's original
            // tile palette (the full sheet is packaged, not a guessed icon).
            assert(kTerrainScatterChoices[rule.tile_offset+n] < 128);
        }
    }
    for (const auto original_marker : {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,50,51,52,100,101,102,103,104,105,106,107,108,109}) {
        assert(markers.count(original_marker)>0);
    }
    return 0;
}
