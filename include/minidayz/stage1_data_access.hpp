#pragma once

#include "minidayz/generated/stage1_data.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace minidayz::stage1 {

// Read-only access to the source-backed Construct export tables.
// Returned pointers refer to static constexpr data and remain valid for the
// lifetime of the program.
const LayoutDef* find_layout(std::string_view name) noexcept;
const ObjectDef* find_object(std::uint16_t index) noexcept;
const LayerDef* find_layer(std::uint16_t layout, std::uint16_t layer) noexcept;
const AudioDef* find_audio(std::string_view filename) noexcept;

// Range helpers return a pointer/count pair into the original generated
// tables. A zero count is valid; callers must not dereference a null pointer.
struct TableRange {
    const InstanceDef* data{};
    std::size_t size{};
};
struct AnimationRange {
    const AnimationDef* data{};
    std::size_t size{};
};
TableRange instances_for_layout(std::uint16_t layout) noexcept;
TableRange instances_for_layer(std::uint16_t layout, std::uint16_t layer) noexcept;
AnimationRange animations_for_object(std::uint16_t object) noexcept;

// Checks structural invariants of the generated source snapshot. This does
// not claim that Construct event behavior has been ported.
struct DataValidation {
    std::size_t invalid_object_indices{};
    std::size_t invalid_layout_indices{};
    std::size_t invalid_layer_indices{};
    std::size_t invalid_instance_ranges{};
    std::size_t invalid_animation_objects{};
    std::size_t invalid_audio_entries{};
    bool ok() const noexcept;
};
DataValidation validate_data() noexcept;

} // namespace minidayz::stage1
