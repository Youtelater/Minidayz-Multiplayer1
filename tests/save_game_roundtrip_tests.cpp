#include "minidayz/save_game.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <limits>
using namespace minidayz;
using namespace minidayz::items;
using namespace minidayz::persistence;

// v5-v20 ground loot on-disk structure: 64 entries, independent of v21.
struct PreviousGroundLootV20 {
    std::array<GroundLootEntry,64> entries{};
    std::uint16_t count{};
    PreviousGroundLootV20& operator=(const GroundLootSnapshot& value) noexcept {
        count=value.count;
        for(std::size_t i=0;i<entries.size();++i)entries[i]=value.entries[i];
        return *this;
    }
};
struct PreviousSaveV5 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{5};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
};

struct PreviousSaveV6 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{6};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
};


struct PreviousSaveV7 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{7};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
};

struct PreviousSaveV9 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{9};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
};

// v10/v11 predate the appended exact equipped wear field. Keep their
// on-disk layout independent of the current v12 SavedGame struct.
struct PreviousSaveV11 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{11};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
};

// Old v12 contains equipment fractions but no Supplies Stash extension.
struct PreviousSaveV12 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{12};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
    EquipmentFractionSnapshot equipment_fraction{};
};

// v13/v14 saved the same six-slot stash layout without Timer_ALL.
struct PreviousSaveV14 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{14};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
    EquipmentFractionSnapshot equipment_fraction{};
    StashStorageSnapshot stashes{};
};

// Source v15 file predated rain timer phase persistence.
struct PreviousSaveV15 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{15};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
    EquipmentFractionSnapshot equipment_fraction{};
    StashStorageSnapshot stashes{};
    SourceDayNightSnapshot source_time{};
};

// Source version 16 predates Check_loot timer persistence.
struct PreviousSaveV16 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{16};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
    EquipmentFractionSnapshot equipment_fraction{};
    StashStorageSnapshot stashes{};
    SourceDayNightSnapshot source_time{};
    SourceRainTimerSnapshot source_rain{};
};

// Previous source v17 file had no pending crafting field.
struct PreviousSaveV17 {
    std::uint32_t magic{kSaveMagic}, version{17}, seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
    EquipmentFractionSnapshot equipment_fraction{};
    StashStorageSnapshot stashes{};
    SourceDayNightSnapshot source_time{};
    SourceRainTimerSnapshot source_rain{};
    SourceLootTimerSnapshot source_loot{};
};
struct LegacyPerkSnapshotV8ForTest {
    std::uint32_t experience_points{};
    std::uint32_t perk_cost{100};
    std::uint16_t perks_learnt_count{};
    std::uint8_t camel_level{};
    std::uint8_t hamster_level{};
};
struct PreviousSaveV8 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{8};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    LegacyPerkSnapshotV8ForTest perks{};
};
static_assert(sizeof(LegacyPerkSnapshotV8ForTest) == 12);

struct LegacyInventorySlotForTest {
    std::uint16_t item_id{};
    std::uint16_t count{};
};
struct LegacyInventoryForTest {
    std::array<LegacyInventorySlotForTest, kMaximumInventorySlotCount> slots{};
    std::uint16_t capacity{static_cast<std::uint16_t>(kInventorySlotCount)};
    std::uint16_t equipped_backpack{};
};
struct LegacyEquipmentForTest {
    std::array<std::uint16_t, kEquipmentSlotCount> item_ids{};
};
static_assert(sizeof(LegacyInventorySlotForTest) == 4);
static_assert(sizeof(LegacyInventoryForTest) == 132);
static_assert(sizeof(LegacyEquipmentForTest) == 14);

// Older native save files wrote this layout without a per-field codec.
// Replay each version through the same public loader used on the 3DS.
void write_legacy_save(const char* path, std::uint32_t version,
                       const PlayerState& player) {
    auto* file = std::fopen(path, "wb");
    assert(file);
    const std::uint32_t header[] = {kSaveMagic, version, 75};
    assert(std::fwrite(header, sizeof(header), 1, file) == 1);
    assert(std::fwrite(&player, sizeof(player), 1, file) == 1);
    std::size_t bytes = sizeof(header) + sizeof(player);
    if (version == 2 || version == 3) {
        LegacyInventoryForTest inv{};
        inv.slots[0] = {11, 2};
        assert(std::fwrite(&inv, sizeof(inv), 1, file) == 1);
        bytes += sizeof(inv);
        if (version == 3) {
            LegacyEquipmentForTest equipment{};
            assert(std::fwrite(&equipment, sizeof(equipment), 1, file) == 1);
            bytes += sizeof(equipment);
        }
    } else if (version == 4) {
        InventorySnapshot inv{};
        inv.slots[0] = {11, 2, 67};
        EquipmentSnapshot equipment{};
        assert(std::fwrite(&inv, sizeof(inv), 1, file) == 1);
        assert(std::fwrite(&equipment, sizeof(equipment), 1, file) == 1);
        bytes += sizeof(inv) + sizeof(equipment);
    }
    if (version >= 2 && version <= 4) {
        static constexpr unsigned char zeros[3]{};
        const std::size_t padding = (4 - bytes % 4) % 4;
        assert(std::fwrite(zeros, 1, padding, file) == padding);
    }
    assert(std::fclose(file) == 0);
}


struct PreviousSaveV19 {
    std::uint32_t magic{kSaveMagic},version{19},seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
    EquipmentFractionSnapshot equipment_fraction{};
    StashStorageSnapshot stashes{};
    SourceDayNightSnapshot source_time{};
    SourceRainTimerSnapshot source_rain{};
    SourceLootTimerSnapshot source_loot{};
    PendingCraftSnapshot pending_craft{};
    std::uint8_t world_level{};
};

// v20 save has the original 64-cell array and the four-byte quick-pistol.
struct PreviousSaveV20 {
    std::uint32_t magic{kSaveMagic},version{20},seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    PreviousGroundLootV20 ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
    SatchelSnapshot satchel{};
    EquipmentFractionSnapshot equipment_fraction{};
    StashStorageSnapshot stashes{};
    SourceDayNightSnapshot source_time{};
    SourceRainTimerSnapshot source_rain{};
    SourceLootTimerSnapshot source_loot{};
    PendingCraftSnapshot pending_craft{};
    std::uint8_t world_level{};
    QuickPistolSnapshot quick_pistol{};
};

int main() {
    constexpr const char* kFile = "stage2_roundtrip_test.sav";
    constexpr const char* kTemp = "stage2_roundtrip_test.tmp";
    std::remove(kFile);
    std::remove(kTemp);

    PlayerState player{};
    player.x = 231.5f;
    player.y = 89.0f;
    player.health = 93;
    player.hunger = 24;
    player.thirst = 78;
    player.temperature = -2.5f;
    player.bleeding = 1.5f;
    player.infection = 2.0f;
    player.facing = PlayerFacing::left;
    player.alive = true;
    InventorySystem inventory;
    EquipmentSystem equipment;
    GroundLootSystem ground;
    TentStorageSystem tents;
    assert(inventory.add(454, 1, 0, 81));
    assert(inventory.equip_backpack_from_slot(0));
    assert(inventory.capacity() == 18);
    assert(inventory.add(350, 1, 0, 66));
    assert(equipment.equip_from_inventory(inventory, 0, EquipmentSlot::outerwear));
    assert(inventory.add(456, 1, 0, 83));
    assert(tents.deploy_from_inventory(inventory, 0, 300, 400));
    assert(inventory.add(18, 2, 0, 75));
    assert(tents.store_from_inventory(inventory, 0, 0));
    assert(ground.add({11, 2, 67}, 150.0f, 600.0f));

    assert(!write_saved_game_to_paths(0, player, inventory, equipment,
                                      ground, tents, kFile, kTemp));
    assert(!write_saved_game_to_paths(42, player, inventory, equipment,
                                      ground, tents, kFile, kFile));
    assert(write_saved_game_to_paths(42, player, inventory, equipment,
                                    ground, tents, kFile, kTemp));
    SavedGame restored{};
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == kSaveVersion);
    assert(restored.pending_craft.recipe_index == 0xffffu);
    assert(restored.seed == 42);
    assert(restored.player.x == 231.5f && restored.player.bleeding == 1.5f);
    InventorySystem loaded_inventory;
    EquipmentSystem loaded_equipment;
    GroundLootSystem loaded_ground;
    TentStorageSystem loaded_tents;
    assert(loaded_inventory.restore(restored.inventory));
    assert(loaded_equipment.restore(restored.equipment));
    assert(loaded_ground.restore(restored.ground_loot));
    assert(loaded_tents.restore(restored.tents));
    assert(loaded_inventory.equipped_backpack() == 454);
    assert(loaded_inventory.equipped_backpack_condition() == 81);
    assert(loaded_equipment.equipped(EquipmentSlot::outerwear) == 350);
    assert(loaded_equipment.snapshot().conditions[1] == 66);
    assert(loaded_ground.entry(0)->item.condition == 67);
    assert(loaded_tents.tent(0)->items[0].item_id == 18);
    assert(loaded_tents.tent(0)->items[0].count == 2);
    assert(loaded_tents.tent(0)->items[0].condition == 75);
    assert(loaded_tents.tent(0)->condition == 83);

    // Root item_activate(63) starts Edrink(60s), setting PlayerBase var4
    // to +15. A Continue during that timer must restore the remaining
    // source timer, not silently reset the speed modifier or grant 60s.
    stage1::SourceEnergyDrinkBoost source_energy{};
    assert(!source_energy.on_successful_item_use(63,false,false));
    assert(source_energy.on_successful_item_use(63,false,true));
    source_energy.update(12.5f);
    assert(source_energy.remaining()==47.5f);
    assert(write_saved_game_to_paths(46, player, inventory, equipment,
        ground, tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, 0, &source_energy));
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==kSaveVersion && restored.seed==46);
    assert(restored.source_energy_seconds==47.5f);
    stage1::SourceEnergyDrinkBoost resumed_energy{};
    assert(resumed_energy.restore(restored.source_energy_seconds));
    assert(resumed_energy.remaining()==47.5f &&
           resumed_energy.bonus()==15.0f);
    resumed_energy.update(47.5f);
    assert(resumed_energy.bonus()==0.0f);

    // Read the exact old v21 file prefix, without a timer field, through
    // the real migration code. Old saves remain playable with no bonus.
    SavedGame old_source_save=restored;
    old_source_save.version=21u;
    std::FILE* previous_file=std::fopen(kFile,"wb");
    assert(previous_file!=nullptr);
    assert(std::fwrite(&old_source_save,1,
        offsetof(SavedGame,source_energy_seconds),previous_file)==
        offsetof(SavedGame,source_energy_seconds));
    assert(std::fclose(previous_file)==0);
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==21u && restored.seed==46);
    assert(restored.source_energy_seconds==0.0f);

    // Delayed original crafting persists through Continue, including subsecond time.
    PendingCraftSnapshot craft{0, 2.25f};
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.pending_craft.recipe_index == 0);
    assert(restored.pending_craft.seconds_remaining == 2.25f);
    // World progression must survive Continue without resetting the
    // survivor to level-one terrain. Level zero remains the old default.
    assert(restored.world_level == 0);
    // The saved file is a regular file, so this temporary path cannot open.
    assert(!write_saved_game_to_paths(44, player, inventory, equipment, ground,
        tents, kFile, "stage2_roundtrip_test.sav/travel.tmp", nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, &craft, 1));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.seed == 43 && restored.world_level == 0);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 22u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 23u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 24u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 25u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 26u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 27u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 28u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 29u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, false, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 30u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, false, false, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 31u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, false, false, false, false, false, false, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 32u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5, nullptr, false, false, false, false, false, false,
        false, false, false, false, false, nullptr, true));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 33u && restored.world_level == 5);
    assert(write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 5));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 34u && restored.world_level == 5);

    assert(!write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft, 6));
    craft.recipe_index = 0xffffu;
    craft.seconds_remaining = 1.0f;
    assert(!write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft));
    craft.recipe_index = 0;
    craft.seconds_remaining = std::numeric_limits<float>::quiet_NaN();
    assert(!write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft));
    craft.recipe_index = 999;
    craft.seconds_remaining = 1.0f;
    assert(!write_saved_game_to_paths(43, player, inventory, equipment, ground,
        tents, kFile, kTemp, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, &craft));

    // v17 continues with all original source timers and an idle craft.
    PreviousSaveV17 old17{};
    old17.seed = 44;
    old17.player = player;
    old17.inventory = inventory.snapshot();
    old17.equipment = equipment.snapshot();
    old17.ground_loot = ground.snapshot();
    old17.tents = tents.snapshot();
    {
        auto* file = std::fopen(kFile, "wb");
        assert(file && std::fwrite(&old17, sizeof(old17), 1, file) == 1);
        assert(std::fclose(file) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 17u && restored.seed == 44);
    assert(restored.pending_craft.recipe_index == 0xffffu);
    assert(restored.pending_craft.seconds_remaining == 0.0f);
    assert(write_saved_game_to_paths(42, player, inventory, equipment,
                                    ground, tents, kFile, kTemp));

    // A failed write must never replace an existing usable save.
    assert(!write_saved_game_to_paths(0, player, inventory, equipment,
                                      ground, tents, kFile, kTemp));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.seed == 42);
    auto broken_player = player;
    broken_player.x = std::numeric_limits<float>::quiet_NaN();
    assert(!write_saved_game_to_paths(43, broken_player, inventory, equipment,
                                      ground, tents, kFile, kTemp));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.seed == 42);
    broken_player = player;
    broken_player.health = 0.0f;
    assert(!write_saved_game_to_paths(43, broken_player, inventory, equipment,
                                      ground, tents, kFile, kTemp));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.seed == 42);

    PreviousSaveV5 old{};
    old.seed = 137;
    old.player = player;
    old.inventory = inventory.snapshot();
    old.equipment = equipment.snapshot();
    old.ground_loot = ground.snapshot();
    {
        auto* f = std::fopen(kFile, "wb");
        assert(f && std::fwrite(&old, sizeof(old), 1, f) == 1);
        assert(std::fclose(f) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 5);
    assert(restored.seed == 137 && restored.ground_loot.count == 1);
    assert(restored.tents.count == 0);

    // Version 6 includes tent storage, but not deferred original item Use
    // actions. The upgrade must preserve every stored tent and item condition.
    PreviousSaveV6 old6{};
    old6.seed = 253;
    old6.player = player;
    old6.inventory = inventory.snapshot();
    old6.equipment = equipment.snapshot();
    old6.ground_loot = ground.snapshot();
    old6.tents = tents.snapshot();
    {
        auto* f = std::fopen(kFile, "wb");
        assert(f && std::fwrite(&old6, sizeof(old6), 1, f) == 1);
        assert(std::fclose(f) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 6 && restored.seed == 253);
    assert(restored.tents.count == 1);
    assert(restored.tents.tents[0].items[0].condition == 75);
    assert(restored.pending_item_use.pending_medical_item_id == 0);

    // Current save format retains an in-flight 2-second bandage action.
    ItemUseRuntime pending{};
    assert(pending.begin(18));
    assert(pending.snapshot().seconds_remaining == 2.0f);
    assert(write_saved_game_to_paths(89, player, inventory, equipment, ground,
                                    tents, kFile, kTemp, &pending));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == kSaveVersion && restored.seed == 89);
    assert(restored.pending_item_use.pending_medical_item_id == 18);
    assert(restored.pending_item_use.seconds_remaining == 2.0f);
    assert(restored.perks.perk_cost == 100u);
    ItemUseRuntime restored_pending{};
    assert(restored_pending.restore(restored.pending_item_use));
    PlayerSystem after_restore(player);
    after_restore.set_bleeding(2.0f, 10.0f);
    restored_pending.update(1.0f, after_restore);
    assert(after_restore.state().bleeding == 2.0f);
    restored_pending.update(1.0f, after_restore);
    assert(after_restore.state().bleeding == 0.0f);

    // Original XP and two-level perks survive a v8 save and restore intact.
    SourcePerkSystem source_perks{};
    assert(source_perks.award_experience(100u));
    assert(source_perks.buy(SourcePerk::camel));
    assert(source_perks.award_experience(120u));
    assert(source_perks.buy(SourcePerk::camel));
    assert(source_perks.item_use_flags().camel_active);
    assert(!source_perks.item_use_flags().hamster_active);
    assert(write_saved_game_to_paths(90, player, inventory, equipment, ground,
                                    tents, kFile, kTemp, &pending, &source_perks));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == kSaveVersion && restored.seed == 90);
    SourcePerkSystem perk_restore{};
    assert(perk_restore.restore(restored.perks));
    assert(perk_restore.level(SourcePerk::camel) == 2);
    assert(perk_restore.snapshot().perk_cost == 360);
    assert(perk_restore.item_use_flags().camel_active);
    assert(perk_restore.award_experience(1000));
    assert(perk_restore.buy(SourcePerk::regen));
    assert(perk_restore.level(SourcePerk::regen) == 1);
    auto tampered = perk_restore.snapshot();
    tampered.perk_cost = 100;
    assert(!perk_restore.restore(tampered));
    assert(perk_restore.snapshot().perk_cost == 520);
    assert(write_saved_game_to_paths(90, player, inventory, equipment, ground,
                                    tents, kFile, kTemp, &pending, &source_perks));

    // A v8 save must migrate all old fields without inventing purchases
    // in the other fourteen newly persisted original perk variables.
    PreviousSaveV8 old8{};
    old8.seed = 90;
    old8.player = player;
    old8.inventory = inventory.snapshot();
    old8.equipment = equipment.snapshot();
    old8.ground_loot = ground.snapshot();
    old8.tents = tents.snapshot();
    old8.pending_item_use = pending.snapshot();
    old8.perks.experience_points = source_perks.snapshot().experience_points;
    old8.perks.perk_cost = source_perks.snapshot().perk_cost;
    old8.perks.perks_learnt_count = source_perks.snapshot().perks_learnt_count;
    old8.perks.camel_level = source_perks.snapshot().camel_level;
    old8.perks.hamster_level = source_perks.snapshot().hamster_level;
    {
        auto* file = std::fopen(kFile, "wb");
        assert(file && std::fwrite(&old8, sizeof(old8), 1, file) == 1);
        assert(std::fclose(file) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 8);
    assert(restored.perks.camel_level == 2);
    assert(restored.perks.hamster_level == 0);
    assert(restored.perks.perk_cost == 360);
    for (const auto level : restored.perks.other_levels)
        assert(level == 0);

    // v10 adds simultaneous satchel equipment as a separately persisted
    // source instance. Its own extra slots must survive the native save.
    InventorySystem dual_pack;
    assert(dual_pack.add(450, 1, 0, 76));
    assert(dual_pack.equip_backpack_from_slot(0));
    assert(dual_pack.add(459, 1, 0, 68));
    assert(dual_pack.equip_satchel_from_slot(0));
    assert(dual_pack.capacity() == 24);
    assert(dual_pack.add_to_slot(20, 11, 2));
    assert(dual_pack.add_to_slot(21, 24, 1, 0, 70));
    assert(write_saved_game_to_paths(91, player, dual_pack, equipment, ground,
                                    tents, kFile, kTemp, &pending, &source_perks));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == kSaveVersion && restored.seed == 91);
    assert(restored.satchel.item_id == 459);
    assert(restored.satchel.condition == 68);
    InventorySystem dual_loaded;
    assert(dual_loaded.restore_with_satchel(restored.inventory, restored.satchel));
    assert(dual_loaded.capacity() == 24);
    assert(dual_loaded.is_exclusive_satchel_slot(21));
    assert(dual_loaded.slot(21)->item_id == 24);
    assert(dual_loaded.slot(21)->condition == 70);
    assert(dual_loaded.equipped_backpack() == 450);
    assert(dual_loaded.equipped_satchel() == 459);
    assert(dual_loaded.slot(20)->item_id == 11);
    assert(dual_loaded.slot(20)->count == 2);

    // The previous v9 binary save layout must remain loadable. The new
    // secondary satchel slot migrates as unequipped by default.
    PreviousSaveV9 old9{};
    old9.seed = 92;
    old9.player = player;
    old9.inventory = inventory.snapshot();
    old9.equipment = equipment.snapshot();
    old9.ground_loot = ground.snapshot();
    old9.tents = tents.snapshot();
    old9.pending_item_use = pending.snapshot();
    old9.perks = source_perks.snapshot();
    {
        auto* file = std::fopen(kFile, "wb");
        assert(file && std::fwrite(&old9, sizeof(old9), 1, file) == 1);
        assert(std::fclose(file) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 9 && restored.seed == 92);
    assert(restored.satchel.item_id == 0);
    assert(restored.satchel.condition == 100);

    // Current v11 saves exact 0.25% wear without discarding it on reload.
    InventorySystem fractional_inventory;
    assert(fractional_inventory.add(244, 1, 0, 100));
    assert(fractional_inventory.damage_condition_hundredths(0, 25));
    GroundLootSystem fractional_ground;
    assert(fractional_ground.add({244, 1, 100, 75}, 40.0f, 80.0f));
    assert(write_saved_game_to_paths(93, player, fractional_inventory,
                                    equipment, fractional_ground, tents,
                                    kFile, kTemp));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == kSaveVersion);
    assert(restored.inventory.slots[0].item_id == 244);
    assert(restored.inventory.slots[0].condition == 100);
    assert(restored.inventory.slots[0].fractional_loss == 25);
    assert(restored.ground_loot.entries[0].item.fractional_loss == 75);

    // v10 wrote the same six-byte items, but its sixth byte was unused
    // padding. Simulate old padding containing garbage and ensure migration
    // never treats it as actual damage.
    PreviousSaveV11 old10{};
    old10.version = 10u;
    old10.seed = 94;
    old10.player = player;
    old10.inventory = fractional_inventory.snapshot();
    old10.inventory.slots[0].fractional_loss = 255;
    old10.equipment = equipment.snapshot();
    old10.ground_loot = fractional_ground.snapshot();
    old10.ground_loot.entries[0].item.fractional_loss = 255;
    old10.tents = tents.snapshot();
    old10.tents.tents[0].items[0].fractional_loss = 255;
    old10.pending_item_use = pending.snapshot();
    old10.perks = source_perks.snapshot();
    old10.satchel = {};
    {
        auto* f = std::fopen(kFile, "wb");
        assert(f && std::fwrite(&old10, sizeof(old10), 1, f) == 1);
        assert(std::fclose(f) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 10u && restored.seed == 94);
    assert(restored.inventory.slots[0].fractional_loss == 0);
    assert(restored.ground_loot.entries[0].item.fractional_loss == 0);
    assert(restored.tents.tents[0].items[0].fractional_loss == 0);

    // v11 already stored fractional inventory items, but not the
    // equipped-item fraction extension introduced by v12.
    PreviousSaveV11 old11 = old10;
    old11.version = 11u;
    old11.seed = 95;
    old11.inventory.slots[0].fractional_loss = 25;
    old11.ground_loot.entries[0].item.fractional_loss = 75;
    // v11 interprets item padding as exact wear, unlike the v10 record
    // copied above, whose intentionally invalid bytes were sanitized.
    old11.tents.tents[0].items[0].fractional_loss = 0;
    {
        auto* f = std::fopen(kFile, "wb");
        assert(f && std::fwrite(&old11, sizeof(old11), 1, f) == 1);
        assert(std::fclose(f) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 11u && restored.seed == 95);
    assert(restored.inventory.slots[0].fractional_loss == 25);
    assert(restored.ground_loot.entries[0].item.fractional_loss == 75);
    for (auto fraction : restored.equipment_fraction.fractional_loss)
        assert(fraction == 0);

    // v12 saves must migrate without inventing deployed stashes.
    PreviousSaveV12 old12{};
    old12.seed = 97;
    old12.player = player;
    old12.inventory = fractional_inventory.snapshot();
    old12.equipment = equipment.snapshot();
    old12.ground_loot = fractional_ground.snapshot();
    old12.tents = tents.snapshot();
    old12.pending_item_use = pending.snapshot();
    old12.perks = source_perks.snapshot();
    {
        auto* f=std::fopen(kFile,"wb");
        assert(f && std::fwrite(&old12,sizeof(old12),1,f)==1);
        assert(std::fclose(f)==0);
    }
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==12u && restored.seed==97);
    assert(restored.stashes.count==0);

    // v13 stored preview weather EXPOSURE, not original actor body warmth.
    // The old field must reset on migration, without losing other stats.
    PreviousSaveV14 old13{};
    old13.version = 13u;
    old13.seed = 1301;
    old13.player = player;
    old13.player.temperature = -9.0f;
    old13.inventory = fractional_inventory.snapshot();
    old13.equipment = equipment.snapshot();
    old13.ground_loot = fractional_ground.snapshot();
    old13.tents = tents.snapshot();
    {
        auto* file = std::fopen(kFile, "wb");
        assert(file && std::fwrite(&old13, sizeof(old13), 1, file) == 1);
        assert(std::fclose(file) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 13u && restored.seed == 1301);
    assert(restored.player.temperature == 0.0f);
    assert(restored.inventory.slots[0].fractional_loss == 25);
    // Current v14 persists body temperature as a separate actor stat.
    PlayerState warm_player = player;
    warm_player.temperature = 3.75f;
    assert(write_saved_game_to_paths(1401, warm_player, fractional_inventory,
                                    equipment, fractional_ground, tents,
                                    kFile, kTemp));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == kSaveVersion && restored.seed == 1401);
    assert(restored.source_time.timer_all == 361);
    assert(restored.player.temperature == 3.75f);

    // The exact v14 file is byte-shorter than v15 and must load without
    // interpreting absent time bytes as existing clock state.
    PreviousSaveV14 old14 = old13;
    old14.version = 14;
    old14.seed = 1402;
    old14.player.temperature = 7.5f;
    {
        auto* file = std::fopen(kFile,"wb");
        assert(file && std::fwrite(&old14,sizeof(old14),1,file)==1);
        assert(std::fclose(file)==0);
    }
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==14u && restored.seed==1402);
    assert(restored.player.temperature==7.5f);
    assert(restored.source_time.timer_all==361);

    SourceDayNightClock source_clock;
    assert(source_clock.restore({60, 0.75f, false}));
    assert(write_saved_game_to_paths(1501,warm_player,fractional_inventory,
        equipment,fractional_ground,tents,kFile,kTemp,
        nullptr,nullptr,nullptr,&source_clock));
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==kSaveVersion && restored.seed==1501);
    assert(restored.source_time.timer_all==60);
    assert(restored.source_time.elapsed_seconds==0.75f);
    assert(!restored.source_time.day_mode);

    // v15 retained the world clock but predates Rain_speed/Rain phase.
    PreviousSaveV15 old15{};
    old15.seed=1502;
    old15.player=warm_player;
    old15.inventory=fractional_inventory.snapshot();
    old15.equipment=equipment.snapshot();
    old15.ground_loot=fractional_ground.snapshot();
    old15.tents=tents.snapshot();
    old15.source_time=source_clock.snapshot();
    {
        auto* file=std::fopen(kFile,"wb");
        assert(file && std::fwrite(&old15,sizeof(old15),1,file)==1);
        assert(std::fclose(file)==0);
    }
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==15u && restored.seed==1502);
    assert(restored.source_time.timer_all==60);
    assert(restored.source_rain.timer_firings==0);
    assert(!restored.source_rain.rain.active);

    SourceRainTimer source_rain;
    for(unsigned i=0;i<360;++i)
        assert(source_rain.update(1.0f,2,false,4));
    assert(source_rain.active());
    for(unsigned i=0;i<30;++i)
        assert(source_rain.update(1.0f,2,false,4));
    assert(write_saved_game_to_paths(1601,warm_player,fractional_inventory,
        equipment,fractional_ground,tents,kFile,kTemp,
        nullptr,nullptr,nullptr,&source_clock,&source_rain));
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==kSaveVersion && restored.seed==1601);
    assert(restored.source_rain.rain.active);
    assert(restored.source_rain.timer_firings==1);
    assert(restored.source_rain.elapsed_seconds==30.0f);
    SourceRainTimer resumed_rain;
    assert(resumed_rain.restore(restored.source_rain));
    for(unsigned i=0;i<330;++i)
        assert(resumed_rain.update(1.0f,2,false,1));
    assert(!resumed_rain.active());
    assert(resumed_rain.snapshot().timer_firings==2);

    // v16 Rain and Clock state migrate into the v17 Check_loot schedule.
    PreviousSaveV16 old16{};
    old16.seed=1602;
    old16.player=warm_player;
    old16.inventory=fractional_inventory.snapshot();
    old16.equipment=equipment.snapshot();
    old16.ground_loot=fractional_ground.snapshot();
    old16.tents=tents.snapshot();
    old16.source_time=source_clock.snapshot();
    old16.source_rain=source_rain.snapshot();
    {
        auto* file=std::fopen(kFile,"wb");
        assert(file && std::fwrite(&old16,sizeof(old16),1,file)==1);
        assert(std::fclose(file)==0);
    }
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==16u && restored.seed==1602);
    assert(restored.source_rain.rain.active);
    assert(restored.source_loot.firings==0);

    SourceLootTimer loot_clock;
    for(unsigned i=0;i<1480;++i)
        (void)loot_clock.advance(1.0f);
    for(unsigned i=0;i<280;++i)
        (void)loot_clock.advance(1.0f);
    assert(write_saved_game_to_paths(1701,warm_player,fractional_inventory,
        equipment,fractional_ground,tents,kFile,kTemp,
        nullptr,nullptr,nullptr,&source_clock,&source_rain,&loot_clock));
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==kSaveVersion && restored.seed==1701);
    assert(restored.source_loot.firings==1);
    assert(restored.source_loot.elapsed_seconds==280.0f);
    SourceLootTimer loot_resumed;
    assert(loot_resumed.restore(restored.source_loot));
    for(unsigned i=0;i<1200;++i)
        (void)loot_resumed.advance(1.0f);
    assert(loot_resumed.snapshot().firings==2);

    // v13 retains the six-source-slot Supplies Stash and its exact
    // subpercent item wear independently of permanent Civilian Tent storage.
    InventorySystem stash_inventory;
    StashStorageSystem stash_native;
    assert(stash_inventory.add(461,1));
    assert(stash_native.deploy_from_inventory_checked(
        stash_inventory,0,100,150,nullptr,nullptr));
    assert(stash_inventory.add(26,2,0,100,75));
    assert(stash_native.store_from_inventory(stash_inventory,0,0));
    assert(write_saved_game_to_paths(98,player,stash_inventory,
        equipment,ground,tents,kFile,kTemp,
        nullptr,nullptr,&stash_native));
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==kSaveVersion);
    assert(restored.stashes.count==1);
    assert(restored.stashes.stashes[0].x==100);
    assert(restored.stashes.stashes[0].y==150);
    assert(restored.stashes.stashes[0].items[0].item_id==26);
    assert(restored.stashes.stashes[0].items[0].fractional_loss==75);
    StashStorageSystem stash_loaded;
    assert(stash_loaded.restore(restored.stashes));
    assert(stash_loaded.take_to_inventory(stash_inventory,0,0));
    assert(stash_inventory.count(26)==2);

    // Original firearm wear stays exact in its separate rifle equipment
    // slot across equipping, native saves and
    // unequipping back to an inventory slot.
    InventorySystem held_inv;
    EquipmentSystem held_equipment;
    assert(held_inv.add(244, 1, 0, 100));
    assert(held_equipment.equip_from_inventory(held_inv, 0, EquipmentSlot::weapon));
    assert(held_equipment.apply_verified_wear_action(EquipmentSlot::weapon,
        6904772320561696ULL, 875054325568454ULL, 744011060201717ULL,
        true, false));
    assert(held_equipment.snapshot().conditions[
        static_cast<std::size_t>(EquipmentSlot::weapon)] == 100);
    assert(held_equipment.fraction_snapshot().fractional_loss[
        static_cast<std::size_t>(EquipmentSlot::weapon)] == 25);
    assert(write_saved_game_to_paths(96, player, held_inv,
                                    held_equipment, ground, tents, kFile, kTemp));
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == kSaveVersion && restored.seed == 96);
    EquipmentSystem loaded_held;
    assert(loaded_held.restore_with_fraction(
        restored.equipment,restored.equipment_fraction));
    assert(loaded_held.fraction_snapshot().fractional_loss[
        static_cast<std::size_t>(EquipmentSlot::weapon)] == 25);
    InventorySystem unequipped{};
    assert(loaded_held.unequip_to_inventory(unequipped,EquipmentSlot::weapon,0));
    assert(unequipped.slot(0)->item_id == 244);
    assert(unequipped.slot(0)->fractional_loss == 25);

    // The old v7 binary layout must migrate without losing in-flight
    // medical treatment while leaving newly introduced perk XP at defaults.
    PreviousSaveV7 old7{};
    old7.seed = 322;
    old7.player = player;
    old7.inventory = inventory.snapshot();
    old7.equipment = equipment.snapshot();
    old7.ground_loot = ground.snapshot();
    old7.tents = tents.snapshot();
    old7.pending_item_use = pending.snapshot();
    {
        auto* file = std::fopen(kFile, "wb");
        assert(file && std::fwrite(&old7, sizeof(old7), 1, file) == 1);
        assert(std::fclose(file) == 0);
    }
    assert(load_saved_game_from_path(restored, kFile));
    assert(restored.version == 7 && restored.seed == 322);
    assert(restored.pending_item_use.pending_medical_item_id == 18);
    assert(restored.perks.camel_level == 0 && restored.perks.hamster_level == 0);
    assert(restored.perks.perk_cost == 100);

    // MP quick pistol survives the v21 save and retains fractional wear.
    InventorySystem quick_inventory;
    EquipmentSystem quick_equipment;
    assert(quick_inventory.add(151,1,0,79,21));
    assert(quick_equipment.equip_quick_pistol_from_inventory(quick_inventory,0));
    assert(write_saved_game_to_paths(123,player,quick_inventory,
        quick_equipment,ground,tents,kFile,kTemp));
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==kSaveVersion);
    assert(restored.quick_pistol.item_id==151);
    assert(restored.quick_pistol.condition==79);
    assert(restored.quick_pistol.fractional_loss==21);
    EquipmentSystem restored_quick_equipment;
    assert(restored_quick_equipment.restore_with_fraction(
        restored.equipment,restored.equipment_fraction));
    assert(restored_quick_equipment.restore_quick_pistol(
        restored.quick_pistol));
    assert(restored_quick_equipment.quick_pistol()==151);
    // A malicious/corrupt quick ID is not silently converted to a rifle.
    {
        auto* f=std::fopen(kFile,"r+b");
        assert(f);
        const QuickPistolSnapshot bad{158,100,0};
        assert(std::fseek(f,static_cast<long>(
            offsetof(SavedGame,quick_pistol)),SEEK_SET)==0);
        assert(std::fwrite(&bad,sizeof(bad),1,f)==1);
        assert(std::fclose(f)==0);
        assert(!load_saved_game_from_path(restored,kFile));
    }
    // Legacy v20 must migrate all 64 cells and the quick-pistol without
    // changing the saved on-disk field offsets or losing item conditions.
    PreviousSaveV20 old20{};
    old20.seed=125;
    old20.player=player;
    old20.inventory=inventory.snapshot();
    old20.equipment=equipment.snapshot();
    old20.equipment_fraction=equipment.fraction_snapshot();
    old20.satchel=inventory.satchel_snapshot();
    old20.ground_loot=ground.snapshot();
    old20.tents=tents.snapshot();
    old20.world_level=3;
    old20.quick_pistol={};
    {
        auto* f=std::fopen(kFile,"wb");
        assert(f&&std::fwrite(&old20,sizeof(old20),1,f)==1);
        assert(std::fclose(f)==0);
    }
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==20u && restored.seed==125);
    assert(restored.world_level==3);
    assert(restored.ground_loot.count==ground.count());
    assert(restored.ground_loot.entries[0].item.item_id==11);
    assert(restored.ground_loot.entries[64].item.empty());
    // New v21 capacity can save more than 64 distinct original world drops.
    GroundLootSystem many;
    for(std::size_t i=0;i<300;++i)
        assert(many.add({11,1,kUseSourceItemCondition},
                        150.0f+static_cast<float>(i),600.0f));
    assert(write_saved_game_to_paths(126,player,inventory,equipment,many,
                                    tents,kFile,kTemp));
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==kSaveVersion);
    assert(restored.ground_loot.count==300);
    assert(restored.ground_loot.entries[299].x==449.0f);

    // Old v19 native saves must still load byte-for-byte, with no quick
    // pistol equipped. Their equipment layout must not be reinterpreted.
    PreviousSaveV19 old19{};
    old19.seed=124;
    old19.player=player;
    old19.inventory=inventory.snapshot();
    old19.equipment=equipment.snapshot();
    old19.equipment_fraction=equipment.fraction_snapshot();
    old19.satchel=inventory.satchel_snapshot();
    old19.ground_loot=ground.snapshot();
    old19.tents=tents.snapshot();
    old19.world_level=2;
    {
        auto* f=std::fopen(kFile,"wb");
        assert(f&&std::fwrite(&old19,sizeof(old19),1,f)==1);
        assert(std::fclose(f)==0);
    }
    assert(load_saved_game_from_path(restored,kFile));
    assert(restored.version==19u);
    assert(restored.seed==124 && restored.world_level==2u);
    assert(restored.quick_pistol.item_id==0);
    assert(restored.quick_pistol.condition==100);

    // A crossing stores both the island survivor and the walker's deck position.
    minidayz::BridgePosition crossing{310.0f,118.0f}, resumed{};
    assert(write_saved_game_to_paths(42,player,inventory,equipment,ground,tents,kFile,kTemp,
        nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,2,nullptr,
        false,false,false,false,false,false,false,false,false,false,false,&crossing));
    assert(load_saved_game_from_path(restored,kFile,&resumed));
    assert(resumed.x==crossing.x && resumed.y==crossing.y);
    assert(restored.player.x==player.x && restored.player.y==player.y);
    crossing.area=minidayz::Passage::final_stand;
    assert(write_saved_game_to_paths(42,player,inventory,equipment,ground,tents,kFile,kTemp,
        nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,4,nullptr,
        false,false,false,false,false,false,false,false,false,false,false,&crossing));
    assert(load_saved_game_from_path(restored,kFile,&resumed));
    assert(resumed.area==minidayz::Passage::final_stand && resumed.x==crossing.x);

    assert(write_saved_game_to_paths(42,player,inventory,equipment,ground,tents,kFile,kTemp));
    assert(load_saved_game_from_path(restored,kFile,&resumed));
    assert(!minidayz::BridgeCrossing::valid_position(resumed));

    // Truncation, a spurious trailing byte, NaN, and bad inventory all reject.
    assert(write_saved_game_to_paths(42, player, inventory, equipment,
                                    ground, tents, kFile, kTemp));
    {
        auto* f = std::fopen(kFile, "ab");
        assert(f && std::fputc('Z', f) != EOF);
        assert(std::fclose(f) == 0);
        assert(!load_saved_game_from_path(restored, kFile));
    }
    assert(write_saved_game_to_paths(42, player, inventory, equipment,
                                    ground, tents, kFile, kTemp));
    {
        auto* f = std::fopen(kFile, "r+b");
        assert(f && std::fseek(f, 0, SEEK_END) == 0);
        const auto len = std::ftell(f);
        assert(len > 16);
        assert(std::fclose(f) == 0);
        f = std::fopen(kFile, "wb");
        assert(f && std::fwrite("bad", 1, 3, f) == 3);
        assert(std::fclose(f) == 0);
        assert(!load_saved_game_from_path(restored, kFile));
    }
    // All pre-tent native formats must remain readable after migration.
    for (std::uint32_t version = 1; version <= 4; ++version) {
        write_legacy_save(kFile, version, player);
        assert(load_saved_game_from_path(restored, kFile));
        assert(restored.version == version);
        assert(restored.seed == 75);
        assert(restored.player.x == player.x);
        assert(restored.ground_loot.count == 0);
        assert(restored.tents.count == 0);
        if (version >= 2) {
            assert(restored.inventory.slots[0].item_id == 11);
            assert(restored.inventory.slots[0].count == 2);
            if (version <= 3)
                assert(restored.inventory.slots[0].condition ==
                    source_default_condition(11));
            else
                assert(restored.inventory.slots[0].condition == 67);
        } else {
            assert(restored.inventory.slots[0].empty());
        }
    }
    // Failure must not mutate the last good decoded snapshot.
    const auto previous = restored.seed;
    {
        auto* file = std::fopen(kFile, "wb");
        assert(file && std::fputc('X', file) != EOF);
        assert(std::fclose(file) == 0);
    }
    assert(!load_saved_game_from_path(restored, kFile));
    assert(restored.seed == previous);

    std::remove(kFile);
    std::remove(kTemp);
    return 0;
}
