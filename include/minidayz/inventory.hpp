#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

inline constexpr std::size_t kInventorySlotCount = 15;
inline constexpr std::size_t kMaximumInventorySlotCount = 32;

inline constexpr std::uint8_t kUseSourceItemCondition = 255;
std::uint8_t source_default_condition(std::uint16_t item_id) noexcept;
bool source_item_exists(std::uint16_t item_id) noexcept;
// Exact English item name in root l_eng_items.xml, or nullptr if unknown.
const char* source_item_name(std::uint16_t item_id) noexcept;

struct InventorySlot {
    std::uint16_t item_id{};
    std::uint16_t count{};
    // Ceiling of source variable 6 for equipment. The four-variable
    // gasoline can, MRE, spinning rod and radio have no var6: their
    // explicit source variable 1 value can occupy this same native byte.
    std::uint8_t condition{100};
    // Exact source condition = condition*100 - fractional_loss.
    // Occupies previous padding byte, retaining v10 item layout.
    std::uint8_t fractional_loss{};
    bool empty() const noexcept { return item_id == 0 || count == 0; }
};

struct GroundItem {
    std::uint16_t item_id{};
    std::uint16_t count{};
    // Same per-object state-byte representation as InventorySlot.
    std::uint8_t condition{kUseSourceItemCondition};
    std::uint8_t fractional_loss{};
    bool empty() const noexcept { return item_id == 0 || count == 0; }
};

static_assert(sizeof(InventorySlot) == 6 && sizeof(GroundItem) == 6,
              "Fractional durability cannot change the old item layout");
struct SatchelSnapshot {
    std::uint16_t item_id{};
    std::uint8_t condition{100};
};

struct InventorySnapshot {
    std::array<InventorySlot, kMaximumInventorySlotCount> slots{};
    std::uint16_t capacity{static_cast<std::uint16_t>(kInventorySlotCount)};
    std::uint16_t equipped_backpack{};
    std::uint8_t equipped_backpack_condition{100};
};

// Baseline stack limits ported from the original Check_stack_size event.
// A later perk-aware layer can override these when perk_cybb is active.
std::uint16_t source_max_stack(std::uint16_t item_id) noexcept;
// Original Check_stack_size returns 31,415,926 when perk_cybb is active.
// Native slot counts are uint16_t, so UINT16_MAX is the equivalent safe cap.
std::uint16_t source_max_stack(std::uint16_t item_id, bool perk_cybb) noexcept;
bool is_backpack_item(std::uint16_t item_id) noexcept;
// Inventory slot count granted by each wearable backpack; zero for other items.
std::size_t source_backpack_capacity(std::uint16_t item_id) noexcept;
// Exact original Construct object IDs resolved from MDZItemTable and the
// original project object table. 65535 means the source has no mapping.
std::uint16_t source_construct_object(std::uint16_t item_id) noexcept;
// Object ID with original sprite frames, resolved from the source sprite catalog.
std::uint16_t source_sprite_object(std::uint16_t item_id) noexcept;

class InventorySystem {
public:
    InventorySystem() noexcept = default;

    bool set_capacity(std::size_t capacity) noexcept;
    // Three source-restricted Army/Medical satchel cells. The final three
    // slots of a satchel-extended inventory accept only the original
    // Construct item whitelist for that bag. Normal inventory is unrestricted.
    bool slot_accepts(std::size_t slot_index,
                      std::uint16_t item_id) const noexcept;
    bool is_exclusive_satchel_slot(std::size_t slot_index) const noexcept;

    // Backpack equipment is a real equipment slot. Capacity is resolved
    // internally from the original Construct UI catalog; callers cannot guess it.
    bool equip_backpack_from_slot(std::size_t inventory_slot) noexcept;
    bool unequip_backpack_to_slot(std::size_t inventory_slot) noexcept;
    std::uint16_t equipped_backpack() const noexcept { return equipped_backpack_; }
    std::uint8_t equipped_backpack_condition() const noexcept {
        return equipped_backpack_condition_;
    }
    std::size_t capacity() const noexcept { return capacity_; }
    // Original Satchel/Army Satchel/Medical Satchel can be worn alongside
    // an ordinary backpack. Army/Medical exclusive cells enforce the verified
    // Construct item-ID whitelists for each satchel type.
    bool equip_satchel_from_slot(std::size_t inventory_slot) noexcept;
    bool unequip_satchel_to_slot(std::size_t inventory_slot) noexcept;
    std::uint16_t equipped_satchel() const noexcept { return equipped_satchel_; }
    std::uint8_t equipped_satchel_condition() const noexcept {
        return equipped_satchel_condition_;
    }
    SatchelSnapshot satchel_snapshot() const noexcept {
        return {equipped_satchel_, equipped_satchel_condition_};
    }
    bool restore_with_satchel(const InventorySnapshot& inventory,
                              const SatchelSnapshot& satchel) noexcept;

    // max_stack == 0 selects the original game's baseline rule for item_id.
    bool add(std::uint16_t item_id, std::uint16_t amount,
             std::uint16_t max_stack = 0,
             std::uint8_t condition = kUseSourceItemCondition,
             std::uint8_t fractional_loss = 0) noexcept;
    // Place or merge into one exact slot without silently choosing another.
    bool add_to_slot(std::size_t slot, std::uint16_t item_id,
                     std::uint16_t amount, std::uint16_t max_stack = 0,
                     std::uint8_t condition = kUseSourceItemCondition,
                     std::uint8_t fractional_loss = 0) noexcept;
    bool remove(std::size_t slot, std::uint16_t amount) noexcept;
    // Apply source condition loss; an item is destroyed when condition reaches zero.
    bool damage_condition(std::size_t slot, std::uint8_t loss) noexcept;
    // Fractional original wear (one hundredth of one percent per unit).
    bool damage_condition_hundredths(
        std::size_t slot, std::uint16_t loss_hundredths) noexcept;
    // Source-UID specific action: caller must evaluate Construct ancestry.
    bool apply_verified_wear_action(std::size_t slot,
        std::uint64_t group_uid, std::uint64_t event_uid,
        std::uint64_t action_uid, bool source_guards_satisfied,
        bool perk_wxnj_active) noexcept;
    // Apply an absolute condition value decoded from a source repair action.
    // This deliberately does not invent repair amounts or repair eligibility.
    bool restore_condition(std::size_t slot, std::uint8_t condition) noexcept;
    // Repair the equipped backpack using an original source repair material.
    bool repair_equipped_backpack_from_inventory_slot(
        std::size_t material_slot) noexcept;

    // Ground transfers are atomic: failed pickups/drops leave both sides intact.
    bool pickup(GroundItem& ground, std::uint16_t max_stack = 0) noexcept;
    bool drop(std::size_t inventory_slot, std::uint16_t amount,
              GroundItem& ground, std::uint16_t max_stack = 0) noexcept;

    // amount == 0 moves the complete source stack. Same-item merges obey the
    // original stack cap; different item types swap only as complete stacks.
    bool move(std::size_t from, std::size_t to,
              std::uint16_t amount = 0,
              std::uint16_t max_stack = 0) noexcept;
    // Touchscreen split: move half a stack to one empty slot atomically.
    bool split_half(std::size_t from, std::size_t to) noexcept;

    const InventorySlot* slot(std::size_t index) const noexcept;
    std::uint32_t count(std::uint16_t item_id) const noexcept;
    std::size_t occupied_slots() const noexcept;
    InventorySnapshot snapshot() const noexcept;
    bool restore(const InventorySnapshot& snapshot) noexcept;
    void clear() noexcept {
        slots_ = {};
        capacity_ = kInventorySlotCount;
        equipped_backpack_ = 0;
        equipped_backpack_condition_ = 100;
        equipped_satchel_ = 0;
        equipped_satchel_condition_ = 100;
    }

private:
    std::array<InventorySlot, kMaximumInventorySlotCount> slots_{};
    std::size_t capacity_{kInventorySlotCount};
    std::uint16_t equipped_backpack_{};
    std::uint8_t equipped_backpack_condition_{100};
    std::uint16_t equipped_satchel_{};
    std::uint8_t equipped_satchel_condition_{100};
};

} // namespace minidayz::items
