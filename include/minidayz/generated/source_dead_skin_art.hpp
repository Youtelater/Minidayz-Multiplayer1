#pragma once
#include <cstddef>
#include <cstdint>
namespace minidayz::stage1 {
// Root-original Sprite813/id_N source animation and 193 skin assignment.
// Source Player_skin_type21 (object1262) has no authored 813/id_21.
struct SourceDeadSkinArt {std::uint16_t source_skin;const char* animation;};
inline constexpr std::size_t kSourceDeadSkinCount=20;
inline constexpr SourceDeadSkinArt kSourceDeadSkins[]={
{679,"id_2"},
{680,"id_3"},
{741,"id_4"},
{742,"id_1"},
{743,"id_9"},
{744,"id_8"},
{745,"id_6"},
{746,"id_11"},
{747,"id_12"},
{748,"id_5"},
{749,"id_7"},
{750,"id_10"},
{811,"id_13"},
{825,"id_14"},
{826,"id_15"},
{996,"id_19"},
{997,"id_17"},
{998,"id_18"},
{999,"id_16"},
{1009,"id_20"},
};
inline const char* source_dead_skin_animation(std::uint16_t source_skin) noexcept {
    for(const auto& entry:kSourceDeadSkins)
        if(entry.source_skin==source_skin)return entry.animation;
    return nullptr;
}
} // namespace minidayz::stage1
