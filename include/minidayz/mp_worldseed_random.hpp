#pragma once
#include <cstdint>
namespace minidayz::worldseed {
// Exact 32-bit generator from root mp_worldseed.js _x1ee9cb (Mulberry32).
// Multiplayer world-sync uses this instead of std::mt19937. Native
// singleplayer generation intentionally keeps its existing source RNG
// policy: changing it would invalidate existing native world seeds.
class MpWorldRandom {
public:
    explicit constexpr MpWorldRandom(std::uint32_t seed) noexcept : state_(seed) {}
    std::uint32_t next_u32() noexcept {
        state_+=1831565813u;
        std::uint32_t t=(state_^(state_>>15u))*(1u|state_);
        t=(t+(t^(t>>7u))*(61u|t))^t;
        return t^(t>>14u);
    }
    double next_unit() noexcept {
        return static_cast<double>(next_u32())/4294967296.0;
    }
private:
    std::uint32_t state_{};
};
}
