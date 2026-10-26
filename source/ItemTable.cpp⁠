#include "ItemTable.h"
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

static std::unordered_map<std::string, std::string> g_itemTable;

extern "C" void ItemTable_Load(void) {
    std::ifstream file("romfs:/data/player_data.json");
    if (!file.is_open()) return;

    try {
        nlohmann::json j;
        file >> j;
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (it.value().is_string()) {
                    g_itemTable[it.key()] = it.value().get<std::string>();
                }
            }
        }
    } catch (...) {
        // Fallback if JSON parsing fails
    }
}

std::string ItemTable_GetItemName(const std::string& itemId) {
    auto it = g_itemTable.find(itemId);
    if (it != g_itemTable.end()) {
        return it->second;
    }
    return "Unknown Item";
}
