#include "ItemTable.h"
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

static std::unordered_map<std::string, std::string> g_itemTable;

extern "C" void ItemTable_Load(void) {
    std::ifstream file("romfs:/data/player_data.json");
    if (!file.is_open()) return;

    nlohmann::json j;
    file >> j;
    if (j.contains("items") && j["items"].is_object()) {
        for (auto& el : j["items"].items()) {
            if (el.value().is_string()) {
                g_itemTable[el.key()] = el.value().get<std::string>();
            }
        }
    }
}

std::string ItemTable_GetItemName(const std::string& itemId) {
    auto it = g_itemTable.find(itemId);
    if (it != g_itemTable.end()) {
        return it->second;
    }
    return "Unknown Item";
}
