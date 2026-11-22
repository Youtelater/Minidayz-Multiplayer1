#include "minidayz/source_furniture_interaction.hpp"
#include <cassert>
int main() {
    using namespace minidayz;
    using namespace minidayz::world_source;
    WorldPlacement bed{};
    bed.object=kFurnitureBed;bed.layer=kFurnitureLayer;
    bed.width=37;bed.height=21;bed.opacity=1;bed.runtime_uid=1;
    assert(source_furniture_active(bed));
    bed.layer=44;assert(!source_furniture_active(bed));
    bed.layer=kFurnitureLayer;bed.opacity=0;
    assert(!source_furniture_active(bed));
    assert(source_pump_draw(50.0f,70).consumed==25);
    assert(source_pump_draw(98.0f,70).consumed==2);
    assert(source_pump_draw(15.0f,3).consumed==3);
    assert(!source_pump_draw(100.0f,70).can_use);
    assert(!source_pump_draw(50.0f,0).can_use);
}
