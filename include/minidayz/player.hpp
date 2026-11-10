#pragma once

#include <cstdint>

namespace minidayz {

enum class PlayerFacing : std::uint8_t { down, up, left, right };

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
    bool alive{true};
};

struct PlayerInput {
    float move_x{};
    float move_y{};
    bool run{};
};

// Values must be populated from the source-game audit before gameplay is
// enabled. They intentionally have no guessed Mini DAYZ defaults.
struct PlayerRules {
    float walk_speed{};
    float run_speed{};
    float max_health{};
    float max_hunger{};
    float max_thirst{};
};

using PlayerCollisionQuery = bool (*)(void* context, float x, float y) noexcept;

struct PlayerMoveResult {
    float dx{};
    float dy{};
    bool blocked_x{};
    bool blocked_y{};
};

class PlayerSystem {
public:
    explicit PlayerSystem(const PlayerState& initial) noexcept : state_(initial) {}

    const PlayerState& state() const noexcept { return state_; }
    PlayerState& state_for_setup() noexcept { return state_; }

    PlayerMoveResult update_movement(
        const PlayerInput& input,
        float delta_seconds,
        const PlayerRules& rules,
        PlayerCollisionQuery is_blocked = nullptr,
        void* collision_context = nullptr) noexcept;

    void apply_damage(float amount) noexcept;
    void heal(float amount, float max_health) noexcept;
    void adjust_hunger(float amount, float max_hunger) noexcept;
    void adjust_thirst(float amount, float max_thirst) noexcept;
    void adjust_temperature(float amount) noexcept;
    void set_bleeding(float severity) noexcept;
    void set_infection(float severity) noexcept;

private:
    static float clamp(float value, float low, float high) noexcept;
    PlayerState state_{};
};

} // namespace minidayz
