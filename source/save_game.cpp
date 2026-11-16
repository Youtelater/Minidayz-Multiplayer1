#include "minidayz/save_game.hpp"
#include "minidayz/world.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace minidayz::persistence {

// Legacy struct layouts and migrations are deliberately identical to the
// in-game save writer in the previous 3DS executable.
constexpr const char* kSavePath = "sdmc:/3ds/Minidayz/world.sav";
constexpr const char* kSaveTempPath = "sdmc:/3ds/Minidayz/world.tmp";

struct LegacyInventorySlotV3 {
    std::uint16_t item_id{};
    std::uint16_t count{};
};

struct LegacyInventorySnapshotV3 {
    std::array<LegacyInventorySlotV3, minidayz::items::kMaximumInventorySlotCount> slots{};
    std::uint16_t capacity{static_cast<std::uint16_t>(minidayz::items::kInventorySlotCount)};
    std::uint16_t equipped_backpack{};
};

struct LegacyEquipmentSnapshotV3 {
    std::array<std::uint16_t, minidayz::items::kEquipmentSlotCount> item_ids{};
};

static_assert(sizeof(LegacyInventorySlotV3) == 4, "v3 inventory slot ABI changed");
static_assert(sizeof(LegacyInventorySnapshotV3) == 132, "v3 inventory snapshot ABI changed");
static_assert(sizeof(LegacyEquipmentSnapshotV3) == 14, "v3 equipment snapshot ABI changed");

// The old v5 layout is read as a complete struct, preserving C++ alignment
// padding between EquipmentSnapshot and GroundLootSnapshot.
struct SavedGameV5 {
    std::uint32_t magic{};
    std::uint32_t version{};
    std::uint32_t seed{};
    minidayz::PlayerState player{};
    minidayz::items::InventorySnapshot inventory{};
    minidayz::items::EquipmentSnapshot equipment{};
    minidayz::items::GroundLootSnapshot ground_loot{};
};

// Version 6 introduced native persistent tents, before timed medical actions.
struct SavedGameV6 {
    std::uint32_t magic{};
    std::uint32_t version{};
    std::uint32_t seed{};
    minidayz::PlayerState player{};
    minidayz::items::InventorySnapshot inventory{};
    minidayz::items::EquipmentSnapshot equipment{};
    minidayz::items::GroundLootSnapshot ground_loot{};
    minidayz::items::TentStorageSnapshot tents{};
};

minidayz::items::InventorySnapshot migrate_inventory_v3(
    const LegacyInventorySnapshotV3& legacy) noexcept {
    minidayz::items::InventorySnapshot result{};
    result.capacity = legacy.capacity;
    result.equipped_backpack = legacy.equipped_backpack;
    result.equipped_backpack_condition = legacy.equipped_backpack == 0
        ? 100 : minidayz::items::source_default_condition(legacy.equipped_backpack);
    for (std::size_t i = 0; i < legacy.slots.size(); ++i) {
        const auto item_id = legacy.slots[i].item_id;
        const std::uint8_t condition = item_id == 0
            ? static_cast<std::uint8_t>(100)
            : minidayz::items::source_default_condition(item_id);
        result.slots[i] = {item_id, legacy.slots[i].count, condition};
    }
    return result;
}

minidayz::items::EquipmentSnapshot migrate_equipment_v3(
    const LegacyEquipmentSnapshotV3& legacy) noexcept {
    minidayz::items::EquipmentSnapshot result{};
    result.item_ids = legacy.item_ids;
    for (std::size_t i = 0; i < legacy.item_ids.size(); ++i) {
        result.conditions[i] = legacy.item_ids[i] == 0
            ? 100 : minidayz::items::source_default_condition(legacy.item_ids[i]);
    }
    return result;
}

bool consume_legacy_padding(std::FILE* file, std::size_t bytes) noexcept {
    unsigned char padding[3]{};
    return bytes <= sizeof(padding) &&
        (bytes == 0 || std::fread(padding, 1, bytes, file) == bytes);
}

bool load_saved_game_from_path(SavedGame& save, const char* path) noexcept {
    SavedGame candidate{};
    if (!path || path[0] == '\0') return false;
    std::FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    const bool header_ok =
        std::fread(&candidate.magic, sizeof(candidate.magic), 1, file) == 1 &&
        std::fread(&candidate.version, sizeof(candidate.version), 1, file) == 1 &&
        std::fread(&candidate.seed, sizeof(candidate.seed), 1, file) == 1;
    bool body_ok = false;
    if (header_ok && candidate.version == 1u) {
        // Migrate the previous player-only save format without discarding it.
        body_ok = std::fread(&candidate.player, sizeof(candidate.player), 1, file) == 1;
        candidate.inventory = {};
        candidate.equipment = {};
    } else if (header_ok && (candidate.version == 2u || candidate.version == 3u)) {
        LegacyInventorySnapshotV3 legacy_inventory{};
        body_ok =
            std::fread(&candidate.player, sizeof(candidate.player), 1, file) == 1 &&
            std::fread(&legacy_inventory, sizeof(legacy_inventory), 1, file) == 1;
        if (body_ok) candidate.inventory = migrate_inventory_v3(legacy_inventory);
        if (body_ok && candidate.version == 3u) {
            LegacyEquipmentSnapshotV3 legacy_equipment{};
            body_ok = std::fread(&legacy_equipment, sizeof(legacy_equipment), 1, file) == 1;
            if (body_ok) candidate.equipment = migrate_equipment_v3(legacy_equipment);
        } else {
            candidate.equipment = {};
        }
        if (body_ok) {
            const std::size_t legacy_bytes = 3 * sizeof(std::uint32_t) +
                sizeof(minidayz::PlayerState) + sizeof(LegacyInventorySnapshotV3) +
                (candidate.version == 3u ? sizeof(LegacyEquipmentSnapshotV3) : 0);
            const std::size_t padding = (4 - (legacy_bytes % 4)) % 4;
            body_ok = consume_legacy_padding(file, padding);
        }
    } else if (header_ok && candidate.version == 4u) {
        // Version 4 persisted player, inventory and equipment; ground loot was
        // not part of that format, so it starts empty when migrated.
        body_ok =
            std::fread(&candidate.player, sizeof(candidate.player), 1, file) == 1 &&
            std::fread(&candidate.inventory, sizeof(candidate.inventory), 1, file) == 1 &&
            std::fread(&candidate.equipment, sizeof(candidate.equipment), 1, file) == 1;
        candidate.ground_loot = {};
        if (body_ok) {
            // Version 4 wrote the complete SavedGame struct, including up to
            // three bytes of tail padding required by its 4-byte alignment.
            const std::size_t legacy_bytes = 3 * sizeof(std::uint32_t) +
                sizeof(minidayz::PlayerState) +
                sizeof(minidayz::items::InventorySnapshot) +
                sizeof(minidayz::items::EquipmentSnapshot);
            const std::size_t padding = (4 - (legacy_bytes % 4)) % 4;
            body_ok = consume_legacy_padding(file, padding);
        }
    } else if (header_ok && candidate.version == 5u) {
        SavedGameV5 old{};
        body_ok = std::fseek(file, 0, SEEK_SET) == 0 &&
                  std::fread(&old, sizeof(old), 1, file) == 1;
        if (body_ok) {
            candidate.seed = old.seed;
            candidate.player = old.player;
            candidate.inventory = old.inventory;
            candidate.equipment = old.equipment;
            candidate.ground_loot = old.ground_loot;
            candidate.tents = {};
        }
    } else if (header_ok && candidate.version == 6u) {
        SavedGameV6 old{};
        body_ok = std::fseek(file, 0, SEEK_SET) == 0 &&
                  std::fread(&old, sizeof(old), 1, file) == 1;
        if (body_ok) {
            candidate.seed = old.seed;
            candidate.player = old.player;
            candidate.inventory = old.inventory;
            candidate.equipment = old.equipment;
            candidate.ground_loot = old.ground_loot;
            candidate.tents = old.tents;
            candidate.pending_item_use = {};
        }
    } else if (header_ok && candidate.version == kSaveVersion) {
        // Read the actual on-disk struct, including compiler alignment.
        body_ok = std::fseek(file, 0, SEEK_SET) == 0 &&
                  std::fread(&candidate, sizeof(candidate), 1, file) == 1;
    }
    const bool no_extra_data = std::fgetc(file) == EOF;
    std::fclose(file);
    const bool header_and_body_valid =
        header_ok && body_ok && no_extra_data && candidate.magic == kSaveMagic &&
        candidate.seed != 0 &&
        std::isfinite(candidate.player.x) && std::isfinite(candidate.player.y) &&
        candidate.player.x >= 0.0f && candidate.player.y >= 0.0f &&
        candidate.player.x <= minidayz::WorldSystem::kSourceMapWidth &&
        candidate.player.y <= minidayz::WorldSystem::kSourceMapHeight;
    if (!header_and_body_valid) return false;

    // Validate every persisted subsystem before offering Continue. Previously
    // malformed inventory/equipment/loot could pass the file check and then be
    // silently cleared by the scene constructor, producing a partial restore.
    minidayz::PlayerSystem player_validation(candidate.player);
    minidayz::items::InventorySystem inventory_validation;
    minidayz::items::EquipmentSystem equipment_validation;
    minidayz::items::GroundLootSystem ground_loot_validation;
    minidayz::items::TentStorageSystem tent_validation;
    minidayz::items::ItemUseRuntime item_use_validation;
    if (!player_validation.restore(candidate.player) ||
        !inventory_validation.restore(candidate.inventory) ||
        !equipment_validation.restore(candidate.equipment) ||
        !ground_loot_validation.restore(candidate.ground_loot) ||
        !tent_validation.restore(candidate.tents) ||
        !item_use_validation.restore(candidate.pending_item_use))
        return false;
    // Never replace a caller's valid save with a partially decoded file.
    save = candidate;
    return true;
}

bool write_saved_game_to_paths(std::uint32_t seed,
                      const minidayz::PlayerState& player,
                      const minidayz::items::InventorySystem& inventory,
                      const minidayz::items::EquipmentSystem& equipment,
                      const minidayz::items::GroundLootSystem& ground_loot,
                      const minidayz::items::TentStorageSystem& tents,
                      const char* target_path,
                      const char* temporary_path,
                      const minidayz::items::ItemUseRuntime* item_use) noexcept {
    if (!target_path || !temporary_path || !target_path[0] || !temporary_path[0] ||
        std::strcmp(target_path, temporary_path) == 0 || seed == 0) return false;
    SavedGame save{};
    save.seed = seed;
    save.player = player;
    save.inventory = inventory.snapshot();
    save.equipment = equipment.snapshot();
    save.ground_loot = ground_loot.snapshot();
    save.tents = tents.snapshot();
    save.pending_item_use = item_use ? item_use->snapshot() : minidayz::items::ItemUseRuntimeSnapshot{};
    // Reject an invalid or corrupt in-memory snapshot before touching the
    // temporary file. The existing valid save must remain intact.
    if (!std::isfinite(save.player.x) || !std::isfinite(save.player.y) ||
        save.player.x < 0.0f || save.player.y < 0.0f ||
        save.player.x > minidayz::WorldSystem::kSourceMapWidth ||
        save.player.y > minidayz::WorldSystem::kSourceMapHeight)
        return false;
    minidayz::PlayerSystem player_validation(save.player);
    minidayz::items::InventorySystem inventory_validation;
    minidayz::items::EquipmentSystem equipment_validation;
    minidayz::items::GroundLootSystem ground_validation;
    minidayz::items::TentStorageSystem tent_validation;
    minidayz::items::ItemUseRuntime item_use_validation;
    if (!player_validation.restore(save.player) ||
        !inventory_validation.restore(save.inventory) ||
        !equipment_validation.restore(save.equipment) ||
        !ground_validation.restore(save.ground_loot) ||
        !tent_validation.restore(save.tents) ||
        !item_use_validation.restore(save.pending_item_use))
        return false;
    std::FILE* file = std::fopen(temporary_path, "wb");
    if (!file) return false;
    const bool written = std::fwrite(&save, sizeof(save), 1, file) == 1 &&
                         std::fflush(file) == 0;
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed) {
        (void)std::remove(temporary_path);
        return false;
    }
    // POSIX rename replaces an existing destination atomically when both
    // paths are on the same filesystem. Do not remove the last good save first:
    // a power loss or failed rename must leave Continue usable.
    if (std::rename(temporary_path, target_path) != 0) {
        (void)std::remove(temporary_path);
        return false;
    }
    return true;
}


bool load_saved_game(SavedGame& save) noexcept {
    return load_saved_game_from_path(save, kSavePath);
}

bool write_saved_game(std::uint32_t seed,
                      const PlayerState& player,
                      const items::InventorySystem& inventory,
                      const items::EquipmentSystem& equipment,
                      const items::GroundLootSystem& ground_loot,
                      const items::TentStorageSystem& tents,
                      const items::ItemUseRuntime* item_use) noexcept {
    (void)::mkdir("sdmc:/3ds", 0777);
    (void)::mkdir("sdmc:/3ds/Minidayz", 0777);
    return write_saved_game_to_paths(seed, player, inventory, equipment,
                                    ground_loot, tents, kSavePath, kSaveTempPath, item_use);
}

} // namespace minidayz::persistence
