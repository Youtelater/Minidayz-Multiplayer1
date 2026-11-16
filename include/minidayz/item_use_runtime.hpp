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
    // Original item_activate event 422420671955084:
    // Newspapers (49) are consumed, wait(5), then award 50 XP.
    static constexpr std::uint16_t kOriginalNewspaperItemId = 49;
    static constexpr std::uint32_t kOriginalNewspaperExperience = 50;
    static constexpr std::uint16_t kOriginalScoutHandbookItemId = 116;
    static constexpr std::uint32_t kOriginalScoutHandbookExperience = 200;
    static constexpr float kOriginalScoutHandbookSeconds = 10.0f;
    static constexpr float kOriginalNewspaperSeconds = 5.0f;
    using ExperienceGrant = bool (*)(void* user, std::uint32_t amount) noexcept;

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

    static bool source_reading_rule(std::uint16_t item_id, float& seconds,
                                    std::uint32_t& experience) noexcept {
        seconds = 0.0f;
        experience = 0;
        switch (item_id) {
        case kOriginalNewspaperItemId:
            seconds = kOriginalNewspaperSeconds;
            experience = kOriginalNewspaperExperience;
            return true;
        case kOriginalScoutHandbookItemId:
            seconds = kOriginalScoutHandbookSeconds;
            experience = kOriginalScoutHandbookExperience;
            return true;
        default: return false;
        }
    }

    bool begin_reading(std::uint16_t item_id) noexcept {
        float seconds = 0.0f;
        std::uint32_t experience = 0;
        if (busy() || !source_reading_rule(item_id, seconds, experience))
            return false;
        pending_ = {item_id, seconds};
        return true;
    }

    bool begin_newspaper() noexcept {
        return begin_reading(kOriginalNewspaperItemId);
    }

    void update(float seconds, PlayerSystem& player,
                ExperienceGrant award_experience = nullptr,
                void* experience_context = nullptr) noexcept {
        if (!busy() || !std::isfinite(seconds) || seconds <= 0.0f)
            return;
        if (seconds < pending_.seconds_remaining) {
            pending_.seconds_remaining -= seconds;
            return;
        }
        float reading_seconds = 0.0f;
        std::uint32_t reading_experience = 0;
        if (source_reading_rule(pending_.pending_medical_item_id,
                                reading_seconds, reading_experience)) {
            // Do not lose a source XP award if its recipient is unavailable.
            if (!award_experience ||
                !award_experience(experience_context, reading_experience)) {
                pending_.seconds_remaining = 0.001f;
                return;
            }
            pending_ = {};
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
            float reading_seconds = 0.0f;
            std::uint32_t reading_experience = 0;
            const float max_seconds =
                source_reading_rule(saved.pending_medical_item_id,
                                    reading_seconds, reading_experience)
                    ? reading_seconds
                    : medical_rule(saved.pending_medical_item_id)
                        ? static_cast<float>(
                            medical_rule(saved.pending_medical_item_id)->cooldown_seconds)
                        : 0.0f;
            if (max_seconds == 0.0f || saved.seconds_remaining <= 0.0f ||
                saved.seconds_remaining > max_seconds)
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
