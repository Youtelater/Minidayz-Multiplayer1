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
    bed.opacity=1.0f;
    assert(source_bed_overlap_frame(bed,true)==1);
    assert(source_bed_overlap_frame(bed,false)==0);
    bed.layer=44;
    assert(source_bed_overlap_frame(bed,true)==0);
    bed.layer=kFurnitureLayer;bed.opacity=0.0f;
    assert(source_bed_overlap_frame(bed,true)==0);
    bed.object=kFurniturePump;bed.opacity=1.0f;
    assert(source_bed_overlap_frame(bed,true)==0);
    // Root SleepApproved rejects daytime, freezing source temperature,
    // player var13 (hunger) <10 and var14 (thirst) <10.
    assert(source_bed_sleep_precheck(361,0.0f,50,50)==
           SourceBedSleepPrecheck::daytime);
    assert(source_bed_sleep_precheck(1439,0.0f,50,50)==
           SourceBedSleepPrecheck::daytime);
    assert(source_bed_sleep_precheck(1440,0.0f,50,50)==
           SourceBedSleepPrecheck::passed_known_conditions);
    assert(source_bed_sleep_precheck(360,0.0f,50,50)==
           SourceBedSleepPrecheck::passed_known_conditions);
    assert(source_bed_sleep_precheck(0,-0.01f,50,50)==
           SourceBedSleepPrecheck::cold);
    assert(source_bed_sleep_precheck(0,0.0f,9.99f,50)==
           SourceBedSleepPrecheck::starving);
    assert(source_bed_sleep_precheck(0,0.0f,10.0f,9.99f)==
           SourceBedSleepPrecheck::dehydrated);
    assert(source_bed_sleep_precheck(0,0.0f,10.0f,10.0f)==
           SourceBedSleepPrecheck::passed_known_conditions);
    // Two original Bed recipes, exact source IDs and atomic item use.
    items::InventorySystem bag;
    assert(bag.add(50,1));
    assert(bag.add(714,1));
    assert(!source_bed_materials_available(bag,1));
    assert(source_bed_materials_available(bag,0));
    const auto old=bag.snapshot();
    assert(!source_bed_consume_materials(bag,1));
    assert(bag.snapshot().capacity==old.capacity &&
           bag.count(50)==1 && bag.count(714)==1);
    assert(source_bed_consume_materials(bag,0));
    assert(bag.count(50)==0 && bag.count(714)==0);
    assert(bag.add(50,1) && bag.add(784,1));
    assert(source_bed_consume_materials(bag,1));
    assert(bag.count(50)==0 && bag.count(784)==0);
    assert(!source_bed_site_within_range(0,0,100,0));
    assert(source_bed_site_within_range(0,0,99,0));
    assert(source_bed_sleep_precheck(1441,0.0f,50,50)==
           SourceBedSleepPrecheck::invalid);
}
