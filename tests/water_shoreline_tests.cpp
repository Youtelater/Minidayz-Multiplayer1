#include "minidayz/generated/water_shore_source.hpp"
#include <cassert>
#include <cstdint>
#include <initializer_list>

int main() {
    using namespace minidayz::stage1;
    static_assert(kWaterShoreRuleCount==12,
        "Original Game_events has twelve water shoreline SetTile branches");
    std::uint32_t total=0;
    std::uint32_t shoreline_rules=0;
    bool source_frame_seen[15]{};
    for(const auto& rule:kWaterShoreRules){
        assert(rule.frame<15);
        assert(rule.count>=7 && rule.count<=9);
        assert(rule.uid!=0);
        source_frame_seen[rule.frame]=true;
        bool has_source_water=false;
        for(std::uint16_t i=0;i<rule.count;++i){
            const auto& cond=kWaterShoreChecks[rule.first+i];
            assert(cond.dx>=-1 && cond.dx<=1);
            assert(cond.dy>=-1 && cond.dy<=1);
            if(cond.requires_water)has_source_water=true;
        }
        if(has_source_water)++shoreline_rules;
        total+=rule.count;
    }
    assert(total>=84);
    assert(shoreline_rules>0);
    for(const auto tile : {0,1,2,3,5,6,7,8,9,10,12,13})
        assert(source_frame_seen[tile]);
}
