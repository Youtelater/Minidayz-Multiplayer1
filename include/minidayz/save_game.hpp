#pragma once

#include "minidayz/player.hpp"
#include "minidayz/inventory.hpp"
#include "minidayz/equipment.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/tent_storage.hpp"
#include "minidayz/item_use_runtime.hpp"
#include "minidayz/perks.hpp"
#include <cstdint>

namespace minidayz::persistence {

inline constexpr std::uint32_t kSaveMagic = 0x4d445a33u;
inline constexpr std::uint32_t kSaveVersion = 9u;

struct SavedGame {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{kSaveVersion};
    std::uint32_t seed{};
    minidayz::PlayerState player{};
    minidayz::items::InventorySnapshot inventory{};
    minidayz::items::EquipmentSnapshot equipment{};
    minidayz::items::GroundLootSnapshot ground_loot{};
    minidayz::items::TentStorageSnapshot tents{};
    minidayz::items::ItemUseRuntimeSnapshot pending_item_use{};
    minidayz::items::SourcePerkSnapshot perks{};
};


bool load_saved_game(SavedGame& save) noexcept;
bool load_saved_game_from_path(SavedGame& save, const char* path) noexcept;

bool write_saved_game(std::uint32_t seed,
                      const PlayerState& player,
                      const items::InventorySystem& inventory,
                      const items::EquipmentSystem& equipment,
                      const items::GroundLootSystem& ground_loot,
                      const items::TentStorageSystem& tents,
                      const items::ItemUseRuntime* item_use = nullptr,
                      const items::SourcePerkSystem* perks = nullptr) noexcept;

bool write_saved_game_to_paths(std::uint32_t seed,
                               const PlayerState& player,
                               const items::InventorySystem& inventory,
                               const items::EquipmentSystem& equipment,
                               const items::GroundLootSystem& ground_loot,
                               const items::TentStorageSystem& tents,
                               const char* target_path,
                               const char* temporary_path,
                               const items::ItemUseRuntime* item_use = nullptr,
                               const items::SourcePerkSystem* perks = nullptr) noexcept;
} // namespace minidayz::persistence
