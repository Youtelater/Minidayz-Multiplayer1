#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidayz::world_source {

// The original Construct Map layers have integer z-order indices (see the
// generated stage1::kLayerTable), with higher layers drawn in front.
// Preserve the verified Citro2D convention that larger depth is nearer.
// Reserve a disjoint depth interval for every source layer; otherwise all
// objects at one layer receive identical depth and their authored insertion
// order is lost in the depth buffer.
inline float source_world_layer_depth(
    std::uint16_t layer, std::size_t index_in_layer,
    std::size_t visible_in_layer) noexcept {
    constexpr float kFirstLayerDepth=0.15f;
    constexpr float kLayerStep=0.0125f;
    constexpr float kWithinLayerSpan=0.0100f; // strictly less than kLayerStep
    const float base=kFirstLayerDepth+kLayerStep*static_cast<float>(layer);
    if(visible_in_layer==0)return std::clamp(base,0.15f,0.80f);
    const auto rank=std::min(index_in_layer,visible_in_layer-1);
    const float offset=kWithinLayerSpan*
        (static_cast<float>(rank)+1.0f)/
        (static_cast<float>(visible_in_layer)+1.0f);
    return std::clamp(base+offset,0.15f,0.80f);
}

// Runtime-created instances follow the resident world placement list.
inline float source_dynamic_layer_depth(std::uint16_t layer,
    std::size_t index,std::size_t count) noexcept {
    const float base=source_world_layer_depth(layer,0,0);
    const float rank=count?static_cast<float>(std::min(index,count-1)+1)/
        static_cast<float>(count+1):0.0f;
    return std::min(0.80f,base+0.0100f+0.0020f*rank);
}

// The visible-index query returns unique placement indices in original
// Construct creation order. Stable-sort by the CURRENT layer (which the
// original House_transparenting events can change at runtime), retaining
// source order among sprites in one layer. No guessed Y-sort is applied:
// Construct uses its authored Z order, not a depth-sort by world position.
template <class SourceLayerForIndex>
inline void source_sort_visible_layer_order(
    std::vector<std::uint32_t>& indices,
    SourceLayerForIndex source_layer_for_index) {
    std::stable_sort(indices.begin(),indices.end(),
        [&](std::uint32_t a,std::uint32_t b){
            return source_layer_for_index(a)<source_layer_for_index(b);
        });
}

} // namespace minidayz::world_source
