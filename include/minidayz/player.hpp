#pragma once

#include <cstdint>

namespace minidayz {

enum class PlayerFacing : std::uint8_t { down, up, left, right };
enum class PlayerAnimation : std::uint8_t { idle, walk, run, dead };

// These labels are present in the original Player stats event tree.
enum class PlayerDeathReason : std::uint8_t { none, hunger, thirst, cold, other };

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
    bool alive{true};
    PlayerDeathReason death_reason{PlayerDeathReason::none};
};

struct PlayerInput {
    float move_x{};
    float move_y{};
    bool run{};
};

// All numeric gameplay values are supplied by the source-equivalence layer.
// Zero defaults deliberately avoid inventing Mini DAYZ values while ACE
// command semantics and event timing are still being resolved.
struct PlayerRules {
    float walk_speed{};
    float run_speed{};
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

    PlayerMoveResult update_movement(
        const PlayerInput& input,
        float delta_seconds,
        const PlayerRules& rules,
        PlayerCollisionQuery is_blocked = nullptr,
        void* collision_context = nullptr) noexcept;

    // Advances passive survival effects. Rates and caps are intentionally
    // injected from the audited game rules; this class does not guess them.
    PlayerSurvivalResult update_survival(
        float delta_seconds, const PlayerRules& rules) noexcept;

    void apply_damage(float amount) noexcept;
    // Applies damage from a source event that has an explicit death-log cause.
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
    PlayerState state_{};
};

} // namespace minidayz
