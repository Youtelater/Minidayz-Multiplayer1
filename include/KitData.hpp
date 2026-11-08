#pragma once
#include <string>
#include <vector>

struct EquipmentSlot {
    std::string key;
    std::string translationKey;
    std::string englishName;
    bool useSlot;
    std::vector<int> allowedIds;
};

struct DefaultKitData {
    int firearm;
    int body;
    int legs;
    int melee;
    std::vector<int> pocketItems;
    bool onRespawn;
};
