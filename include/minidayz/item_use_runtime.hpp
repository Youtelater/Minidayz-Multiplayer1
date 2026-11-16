#pragma once

#include "minidayz/player.hpp"
#include "minidayz/generated/item_medical_catalog.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

struct ItemUseRuntimeSnapshot {
    std::uint16_t pending_medical_item_id{};
    float seconds_remaining{};
};

// The original item_activate bandage/rags/tetracycline actions consume their
// item, call Wpn_cooldown, wait 2/5/3 seconds, then clear the respective
// player instance variable. Keep that waiting action in saved game state.
class ItemUseRuntime {
public:
    bool busy() const noexcept { return pending_.pending_medical_item_id != 0; }
    float remaining() const noexcept { return pending_.seconds_remaining; }
    ItemUseRuntimeSnapshot snapshot() const noexcept { return pending_; }

    static const SourceMedicalRule* medical_rule(std::uint16_t item_id) noexcept {
        for (const auto& rule : kSourceMedicalRules)
            if (rule.item_id == item_id) return &rule;
        return nullptr;
    }

    bool can_begin(std::uint16_t item_id) const noexcept {
        return !busy() && medical_rule(item_id) != nullptr;
    }

    bool begin(std::uint16_t item_id) noexcept {
        const auto* rule = medical_rule(item_id);
        if (!rule || busy() || rule->cooldown_seconds == 0) return false;
        pending_ = {item_id, static_cast<float>(rule->cooldown_seconds)};
        return true;
    }

    void update(float seconds, PlayerSystem& player) noexcept {
        if (!busy() || !std::isfinite(seconds) || seconds <= 0.0f)
            return;
        if (seconds < pending_.seconds_remaining) {
            pending_.seconds_remaining -= seconds;
            return;
        }
        const auto* rule = medical_rule(pending_.pending_medical_item_id);
        pending_ = {};
        if (!rule) return;
        if (rule->action == SourceMedicalAction::stop_bleeding)
            player.set_bleeding(0.0f, 0.0f);
        else if (rule->action == SourceMedicalAction::stop_infection)
            player.set_infection(0.0f, 0.0f);
    }

    bool restore(const ItemUseRuntimeSnapshot& saved) noexcept {
        if (!std::isfinite(saved.seconds_remaining)) return false;
        if (saved.pending_medical_item_id == 0) {
            if (saved.seconds_remaining != 0.0f) return false;
        } else {
            const auto* rule = medical_rule(saved.pending_medical_item_id);
            if (!rule || saved.seconds_remaining <= 0.0f ||
                saved.seconds_remaining > static_cast<float>(rule->cooldown_seconds))
                return false;
        }
        pending_ = saved;
        return true;
    }

    void clear() noexcept { pending_ = {}; }

private:
    ItemUseRuntimeSnapshot pending_{};
};

} // namespace minidayz::items
