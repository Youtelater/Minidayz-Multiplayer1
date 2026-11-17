#include "minidayz/generated/source_craft_events.hpp"
#include <cassert>
#include <cstring>

using namespace minidayz::items;

const SourceCraftEvent* find(const char* name) {
    for (const auto& e : kOriginalCraftFunctions)
        if (std::strcmp(e.function,name)==0) return &e;
    return nullptr;
}
int main() {
    static_assert(kOriginalCraftFunctionCount == 19);
    for(const auto& e:kOriginalCraftFunctions) {
        assert(e.source_uid != 0);
        assert(e.result != 0);
        assert(e.source_wait_seconds >= 0.0f);
    }
    const auto* rod=find("Craft_fishing_rod");
    assert(rod && rod->kind==SourceCraftOutputKind::world_object);
    assert(rod->result==837 && rod->source_wait_seconds==4.0f);
    const auto* bed=find("Craft_bed");
    const auto* bearbed=find("Craft_bed1");
    assert(bed && bearbed);
    assert(bed->kind==SourceCraftOutputKind::building_mode);
    assert(bearbed->kind==SourceCraftOutputKind::building_mode);
    assert(bed->result==12 && bearbed->result==12);
    const auto* canvas=find("Craft_tent");
    assert(canvas && canvas->kind==SourceCraftOutputKind::ground_item);
    assert(canvas->result==117 && canvas->source_wait_seconds==5.0f);
    const auto* suppressor=find("Craft_xiaoyinqi");
    assert(suppressor && suppressor->kind==SourceCraftOutputKind::inventory_item);
    assert(suppressor->result==127 && suppressor->source_wait_seconds==5.0f);
    for(const char* name: {"Craft_31","Craft_32","Craft_33","Craft_34"}) {
        const auto* variant=find(name);
        assert(variant && variant->kind==SourceCraftOutputKind::inventory_item);
        assert(variant->result==23 && variant->source_wait_seconds==3.0f);
    }
    for(const char* name: {"Craft_molotov_flare","Craft_molotov_whiskey"}) {
        const auto* e=find(name);
        assert(e && e->kind==SourceCraftOutputKind::inventory_item);
        assert(e->result==102);
    }
    return 0;
}
