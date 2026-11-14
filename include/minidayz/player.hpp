#pragma once

#include <cstdint>

namespace minidayz {

enum class PlayerFacing : std::uint8_t { down, up, left, right };
enum class PlayerAnimation : std::uint8_t { idle, walk, run, dead };
enum class PlayerDeathReason : std::uint8_t { none, hunger, thirst, cold, other };
enum class SurvivalDifficulty : std::uint8_t { novice, regular, veteran };

// Object 679 is the default player skin (player_skin_def_2) in the extracted inventory.
inline constexpr std::uint16_t kSourcePlayerObject = 679;

struct PlayerState {
    float x{};
    float y{};
    float health{};
    float hunger{};
    float thirst{};
    float temperature{};
    float bleeding{};
    float infection{};
    PlayerFacing facing{PlayerFacing::down};
    PlayerAnimation animation{PlayerAnimation::idle};
    std::uint32_t animation_frame{};
    float animation_elapsed{};
    bool alive{true};
    PlayerDeathReason death_reason{PlayerDeathReason::none};
};

// Numeric movement/survival tuning is injected from source-equivalent rules.
// The preview scene may use clearly labeled temporary movement values, but
// source survival rates must remain unset until their event cadence is traced.
// Source rates are expressed per second after converting the original
// five-second hunger/thirst cadence into the native frame-time update.
PlayerRules source_survival_rules(SurvivalDifficulty difficulty) noexcept;

struct PlayerArmorProtection {
    float helmet{};
    float outerwear{};
    float vest{};
    float pants{};
};

struct PlayerRules {
    float walk_speed{};
    float run_speed{};
    float collision_half_width{};
    float collision_half_height{};
    float max_health{};
    float max_hunger{};
    float max_thirst{};
    float max_bleeding{};
    float max_infection{};
    float hunger_drain_per_second{};
    float thirst_drain_per_second{};
    float starvation_damage_per_second{};
    float dehydration_damage_per_second{};
    float bleeding_damage_per_severity_second{};
    float infection_damage_per_severity_second{};
    float regeneration_per_second{};
    float temperature_change_per_second{};
    float min_temperature{};
    float max_temperature{};
};

struct PlayerInput {
    float move_x{};
    float move_y{};
    bool run{};
};

using PlayerCollisionQuery = bool (*)(void* context, float x, float y) noexcept;

struct PlayerMoveResult {
    float dx{};
    float dy{};
    bool blocked_x{};
    bool blocked_y{};
};

struct PlayerSurvivalResult {
    float health_lost{};
    float hunger_lost{};
    float thirst_lost{};
    bool died{};
};

class PlayerSystem {
public:
    explicit PlayerSystem(const PlayerState& initial) noexcept;

    const PlayerState& state() const noexcept { return state_; }
    PlayerState& state_for_setup() noexcept { return state_; }

    // PlayerState is the portable save payload. Restore rejects malformed
    // values instead of allowing NaNs or invalid enum values into runtime.
    PlayerState snapshot() const noexcept { return state_; }
    bool restore(const PlayerState& saved) noexcept;

    PlayerMoveResult update_movement(
        const PlayerInput& input,
        float delta_seconds,
        const PlayerRules& rules,
        PlayerCollisionQuery is_blocked = nullptr,
        void* collision_context = nullptr) noexcept;

    PlayerSurvivalResult update_survival(
        float delta_seconds, const PlayerRules& rules) noexcept;

    // Maps the state/facing to the original player object's animation order:
    // idle_* = 0..3 and run_* = 4..7 for the default skin.
    std::uint16_t source_animation_index() const noexcept;
    std::uint32_t source_animation_frame() const noexcept {
        return state_.animation_frame;
    }

    void apply_damage(float amount) noexcept;
    void apply_damage(float amount, const PlayerArmorProtection& protection) noexcept;
    void apply_damage(float amount, PlayerDeathReason cause) noexcept;
    void kill(PlayerDeathReason reason) noexcept;
    void heal(float amount, float max_health) noexcept;
    void adjust_hunger(float amount, float max_hunger) noexcept;
    void adjust_thirst(float amount, float max_thirst) noexcept;
    void adjust_temperature(float amount, float min_temperature,
                            float max_temperature) noexcept;
    void set_bleeding(float severity, float max_severity) noexcept;
    void set_infection(float severity, float max_severity) noexcept;

private:
    static float clamp(float value, float low, float high) noexcept;
    void update_life_state() noexcept;
    void advance_animation(float delta_seconds) noexcept;
    PlayerState state_{};
};

} // namespace minidayz
