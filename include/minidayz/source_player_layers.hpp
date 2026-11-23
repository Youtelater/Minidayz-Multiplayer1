#pragma once
#include "minidayz/source_world_layer_depth.hpp"
#include <cstdint>
namespace minidayz::stage1 {
// Source data.deobfuscated.js Map world player + 7 equipment layers.
inline constexpr std::uint16_t kSourcePlayerLayer=32;
inline constexpr std::uint16_t kSourcePlayerPantsLayer=33;
inline constexpr std::uint16_t kSourcePlayerHandsLayer=34;
inline constexpr std::uint16_t kSourcePlayerClothingLayer=35;
inline constexpr std::uint16_t kSourcePlayerVestLayer=36;
inline constexpr std::uint16_t kSourcePlayerHelmetLayer=37;
inline constexpr std::uint16_t kSourcePlayerBackpackLayer=38;
inline constexpr std::uint16_t kSourcePlayerWeaponLayer=39;
inline float source_player_layer_depth(std::uint16_t layer) noexcept {
    return world_source::source_world_layer_depth(layer,0,0);
}
} // namespace minidayz::stage1
