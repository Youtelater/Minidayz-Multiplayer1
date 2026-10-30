#include <string>
#include <vector>
#include <unordered_map>

// Enum for all placeable structures from mp_structures.js
enum class StructureType {
    BARRICADE,
    BARBED_WIRE,
    TENT,
    SNARE,
    SNARE_II,
    HIDE_BUSH,
    FENCE,
    FENCE_II,
    BED,
    GENERATOR,
    LAMP,
    FEEDER,
    BEAR_TRAP,
    CAMPFIRE,
    FARM_PLOT,
    PLANT_PEPPER,
    PLANT_ZUCCHINI,
    PLANT_TOMATO,
    C4_CHARGE,
    AIRDROP_BOX
};

// Properties for a structure definition
struct StructureConfig {
    std::string id;
    std::string name;
    int maxHp;
    bool isPlant;
    bool producesLight;
    bool isContainer;
};

// Global lookup table matching the JS object
const std::unordered_map<StructureType, StructureConfig> STRUCTURE_DATABASE = {
    { StructureType::BARRICADE,     { "t567",  "Barricade",        100, false, false, false } },
    { StructureType::BARBED_WIRE,   { "t572",  "Barbed Wire",      50,  false, false, false } },
    { StructureType::TENT,          { "t398",  "Tent",             200, false, false, true  } },
    { StructureType::CAMPFIRE,      { "t262",  "Campfire",         30,  false, true,  false } },
    { StructureType::GENERATOR,     { "t1179", "Generator",        150, false, true,  false } },
    { StructureType::FARM_PLOT,     { "t1176", "Farm Plot",        40,  false, false, false } },
    { StructureType::PLANT_PEPPER,  { "t559",  "Pepper Plant",     20,  true,  false, false } },
    { StructureType::PLANT_TOMATO,  { "t561",  "Tomato Plant",     20,  true,  false, false } },
    { StructureType::C4_CHARGE,     { "t1337", "C4 Charge",        10,  false, false, false } },
    { StructureType::AIRDROP_BOX,   { "t796",  "Airdrop Box",      300, false, false, true  } }
};
