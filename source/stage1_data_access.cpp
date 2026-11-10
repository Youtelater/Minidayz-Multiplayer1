#include "minidayz/stage1_data_access.hpp"

#include <cstring>

namespace minidayz::stage1 {
namespace {
bool equal(const char* lhs, std::string_view rhs) noexcept {
    return lhs && std::strlen(lhs) == rhs.size() &&
           std::memcmp(lhs, rhs.data(), rhs.size()) == 0;
}
bool valid_range(std::uint32_t first, std::uint32_t count,
                 std::size_t total) noexcept {
    return first <= total && count <= total - first;
}
}

const LayoutDef* find_layout(std::string_view name) noexcept {
    for (const auto& item : kLayoutTable)
        if (equal(item.name, name)) return &item;
    return nullptr;
}

const ObjectDef* find_object(std::uint16_t index) noexcept {
    if (index >= kObjectCount) return nullptr;
    return &kObjects[index];
}

const LayerDef* find_layer(std::uint16_t layout, std::uint16_t layer) noexcept {
    for (const auto& item : kLayerTable)
        if (item.layout == layout && item.index == layer) return &item;
    return nullptr;
}

const AudioDef* find_audio(std::string_view filename) noexcept {
    for (const auto& item : kAudio)
        if (equal(item.filename, filename)) return &item;
    return nullptr;
}

TableRange instances_for_layout(std::uint16_t layout) noexcept {
    if (layout >= kLayoutCount) return {};
    const auto& item = kLayoutTable[layout];
    if (!valid_range(item.first, item.count, kInstanceCount)) return {};
    return {kInstances + item.first, item.count};
}

TableRange instances_for_layer(std::uint16_t layout, std::uint16_t layer) noexcept {
    const auto* item = find_layer(layout, layer);
    if (!item || !valid_range(item->first, item->count, kInstanceCount)) return {};
    return {kInstances + item->first, item->count};
}

AnimationRange animations_for_object(std::uint16_t object) noexcept {
    if (object >= kObjectCount) return {};
    // The generated animation table is grouped by source object index.
    std::size_t first = 0;
    while (first < kAnimationCount && kAnimations[first].object < object) ++first;
    std::size_t last = first;
    while (last < kAnimationCount && kAnimations[last].object == object) ++last;
    return {last == first ? nullptr : kAnimations + first, last - first};
}

bool DataValidation::ok() const noexcept {
    return invalid_object_indices == 0 && invalid_layout_indices == 0 &&
           invalid_layer_indices == 0 && invalid_instance_ranges == 0 &&
           invalid_animation_objects == 0 && invalid_audio_entries == 0;
}

DataValidation validate_data() noexcept {
    DataValidation result{};
    for (std::size_t i = 0; i < kObjectCount; ++i)
        if (kObjects[i].index != i) ++result.invalid_object_indices;
    for (std::size_t i = 0; i < kLayoutCount; ++i)
        if (!valid_range(kLayoutTable[i].first, kLayoutTable[i].count, kInstanceCount))
            ++result.invalid_layout_indices;
    for (const auto& layer : kLayerTable) {
        if (layer.layout >= kLayoutCount) ++result.invalid_layer_indices;
        if (!valid_range(layer.first, layer.count, kInstanceCount))
            ++result.invalid_instance_ranges;
    }
    for (const auto& instance : kInstances) {
        if (instance.object >= kObjectCount) ++result.invalid_object_indices;
        if (instance.layout >= kLayoutCount) {
            ++result.invalid_layout_indices;
            continue;
        }
        // Instance.layer is a layout-local layer index, not an index into
        // the global 360-entry layer table. Validate the actual pair.
        if (!find_layer(instance.layout, instance.layer))
            ++result.invalid_layer_indices;
    }
    for (const auto& animation : kAnimations)
        if (animation.object >= kObjectCount) ++result.invalid_animation_objects;
    for (const auto& audio : kAudio)
        if (!audio.filename || audio.filename[0] == '\0') ++result.invalid_audio_entries;
    return result;
}
} // namespace minidayz::stage1
