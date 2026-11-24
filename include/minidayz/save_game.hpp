#pragma once

#include "minidayz/player.hpp"
#include "minidayz/inventory.hpp"
#include "minidayz/equipment.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/tent_storage.hpp"
#include "minidayz/stash_storage.hpp"
#include "minidayz/source_day_night.hpp"
#include "minidayz/source_rain_timer.hpp"
#include "minidayz/source_loot_timer.hpp"
#include "minidayz/item_use_runtime.hpp"
#include "minidayz/source_energy_drink_boost.hpp"
#include "minidayz/perks.hpp"
#include <cstdint>

namespace minidayz::persistence {

inline constexpr std::uint32_t kSaveMagic = 0x4d445a33u;
inline constexpr std::uint32_t kSaveVersion = 24u; // v24 restores the original late-map horizontal road pass.
// Preserve the original Construct wait when a craft is interrupted by autosave.
struct PendingCraftSnapshot {
    std::uint16_t recipe_index{0xffffu};
    float seconds_remaining{};
};

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
    // v10 adds the separate satchel. v11 interprets previously unused item
    // padding bytes as exact fractional source durability. The on-disk item
    // ABI remains six bytes, and older versions migrate with zero fraction.
    minidayz::items::SatchelSnapshot satchel{};
    // v12 adds independently saved subpercent condition for equipped slots,
    // leaving every older EquipmentSnapshot binary layout unchanged.
    minidayz::items::EquipmentFractionSnapshot equipment_fraction{};
    // v13 adds independent six-slot Supplies Stash positions and cargo.
    // v14 keeps the same layout but switches PlayerState.temperature from the
    // old preview weather exposure to the original persisted BODY warmth.
    minidayz::items::StashStorageSnapshot stashes{};
    // v15 appends the original 1.5-second Timer_ALL and its phase state.
    minidayz::items::SourceDayNightSnapshot source_time{};
    // v16 persists the original 360-second Rain timer and active phase.
    minidayz::items::SourceRainTimerSnapshot source_rain{};
    // v17 adds the original repeated Check_loot event timer (1480s).
    minidayz::items::SourceLootTimerSnapshot source_loot{};
    PendingCraftSnapshot pending_craft{};
    // v19 persists the original CurrentLevel for multi-map terrain.
    std::uint8_t world_level{};
    // v20 adds the MP handgun quick slot without altering the legacy
    // EquipmentSnapshot or earlier binary save layouts.
    minidayz::items::QuickPistolSnapshot quick_pistol{};
    // v22 preserves the original 60s Edrink / PlayerBase var4 boost.
    float source_energy_seconds{};
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
                      const items::SourcePerkSystem* perks = nullptr,
                      const items::StashStorageSystem* stashes = nullptr,
                      const items::SourceDayNightClock* source_time = nullptr,
                      const items::SourceRainTimer* source_rain = nullptr,
                      const items::SourceLootTimer* source_loot = nullptr,
                      const PendingCraftSnapshot* pending_craft = nullptr,
                      std::uint8_t world_level = 0,
                      const stage1::SourceEnergyDrinkBoost* source_energy = nullptr,
                      bool legacy_world_generation = false,
                      bool legacy_split_roads = false) noexcept;

bool write_saved_game_to_paths(std::uint32_t seed,
                               const PlayerState& player,
                               const items::InventorySystem& inventory,
                               const items::EquipmentSystem& equipment,
                               const items::GroundLootSystem& ground_loot,
                               const items::TentStorageSystem& tents,
                               const char* target_path,
                               const char* temporary_path,
                               const items::ItemUseRuntime* item_use = nullptr,
                               const items::SourcePerkSystem* perks = nullptr,
                      const items::StashStorageSystem* stashes = nullptr,
                      const items::SourceDayNightClock* source_time = nullptr,
                      const items::SourceRainTimer* source_rain = nullptr,
                      const items::SourceLootTimer* source_loot = nullptr,
                      const PendingCraftSnapshot* pending_craft = nullptr,
                      std::uint8_t world_level = 0,
                      const stage1::SourceEnergyDrinkBoost* source_energy = nullptr,
                      bool legacy_world_generation = false,
                      bool legacy_split_roads = false) noexcept;
} // namespace minidayz::persistence
