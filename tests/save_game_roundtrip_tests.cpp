#include "minidayz/save_game.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
using namespace minidayz;
using namespace minidayz::items;
using namespace minidayz::persistence;

struct PreviousSaveV5 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{5};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    GroundLootSnapshot ground_loot{};
};

struct PreviousSaveV6 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{6};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    GroundLootSnapshot ground_loot{};
    TentStorageSnapshot tents{};
};


struct PreviousSaveV7 {
    std::uint32_t magic{kSaveMagic};
    std::uint32_t version{7};
    std::uint32_t seed{};
    PlayerState player{};
    InventorySnapshot inventory{};
    EquipmentSnapshot equipment{};
    GroundLootSnapshot ground_loot{};
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
    GroundLootSnapshot ground_loot{};
    TentStorageSnapshot tents{};
    ItemUseRuntimeSnapshot pending_item_use{};
    SourcePerkSnapshot perks{};
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
    GroundLootSnapshot ground_loot{};
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
