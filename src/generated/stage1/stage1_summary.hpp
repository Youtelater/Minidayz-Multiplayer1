#pragma once
#include <cstddef>
#include <cstdint>
namespace minidayz::stage1 {
inline constexpr std::uint32_t kSourceBytes = 14558329;
inline constexpr std::uint32_t kProjectSections = 29;
inline constexpr std::uint32_t kProjectCanvasWidth = 1024, kProjectCanvasHeight = 768;
inline constexpr std::uint32_t kObjectDefinitions = 1429;
inline constexpr std::uint32_t kLayouts = 8, kLayers = 360, kPlacedInstances = 5256;
inline constexpr std::uint32_t kAnimationDefinitions = 6338;
inline constexpr std::uint32_t kImageFrameReferences = 14505;
inline constexpr std::uint32_t kAudioResources = 489;
inline constexpr std::uint32_t kEventSheets = 6, kNestedEventNodes = 22343;
inline constexpr std::uint32_t kEventVariables = 1017, kNamedGroups = 220;
inline constexpr std::uint32_t kDistinctConditionCommandIds = 115;
inline constexpr std::uint32_t kDistinctActionCommandIds = 206;
struct EventSheetSummary { const char* name; std::uint32_t top_level_entries, event_nodes, variables; };
inline constexpr EventSheetSummary kEventSheetSummary[] = {
 {"Game_events",518,21348,888}, {"Menu_Events",153,923,100},
 {"Login_events",27,60,8}, {"Loading_events",15,8,9},
 {"Everywere",4,4,1}, {"Tutorial_events",13,0,11}
};
} // namespace minidayz::stage1
