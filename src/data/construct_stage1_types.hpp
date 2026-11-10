#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

namespace minidayz::source {
struct Transform {
    float x{}, y{}, z{}, width{}, height{}, angle{}, opacity{1.0f}, scale_x{1.0f};
    float origin_x{0.5f}, origin_y{0.5f};
};
struct ObjectDefinition {
    std::uint32_t source_index{};
    std::uint32_t construct_type{};
    std::string_view name;
};
struct PlacedInstance {
    std::uint32_t layout_index{}, layer_index{}, object_index{}, uid{};
    Transform transform;
};
struct LayerDefinition {
    std::uint32_t layout_index{}, layer_index{};
    std::string_view name;
    std::uint32_t first_instance{}, instance_count{};
    bool visible{true};
};
struct LayoutDefinition {
    std::string_view name, event_sheet;
    float width{}, height{};
    std::uint32_t first_instance{}, instance_count{};
};
struct AnimationDefinition {
    std::uint32_t object_index{};
    std::string_view name;
    std::uint32_t frame_count{};
    float playback_speed{};
    bool loop{};
};
struct AudioResource {
    std::string_view filename;
    std::uint64_t source_bytes{};
};
} // namespace minidayz::source
