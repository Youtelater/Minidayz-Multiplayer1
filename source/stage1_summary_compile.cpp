// Compile-only Stage 1 metadata assertions. Kept under source/ per project convention.
#include "minidayz/stage1_summary.hpp"
#include "minidayz/generated/stage1_data.hpp"
static_assert(minidayz::stage1::kProjectSections==29);
static_assert(minidayz::stage1::kObjectDefinitions==1429);
static_assert(minidayz::stage1::kPlacedInstances==5256);
static_assert(minidayz::stage1::kAnimationDefinitions==6338);
static_assert(minidayz::stage1::kEventSheets==6);
static_assert(minidayz::stage1::kNestedEventNodes==22343);
static_assert(minidayz::stage1::kEventVariables==1017);

static_assert(minidayz::stage1::kImageFrameReferences==14505);
static_assert(minidayz::stage1::kAudioResources==489);
static_assert(minidayz::stage1::kNamedGroups==220);

static_assert(minidayz::stage1::kObjectCount==minidayz::stage1::kObjectDefinitions);
static_assert(minidayz::stage1::kLayoutCount==minidayz::stage1::kLayouts);
static_assert(minidayz::stage1::kLayerCount==minidayz::stage1::kLayers);
static_assert(minidayz::stage1::kInstanceCount==minidayz::stage1::kPlacedInstances);
static_assert(minidayz::stage1::kAnimationCount==minidayz::stage1::kAnimationDefinitions);
static_assert(minidayz::stage1::kAudioCount==minidayz::stage1::kAudioResources);
static_assert(minidayz::stage1::kEventSheetCount==minidayz::stage1::kEventSheets);
static_assert(minidayz::stage1::kEventCount==minidayz::stage1::kNestedEventNodes);
static_assert(minidayz::stage1::kVariableCount==minidayz::stage1::kEventVariables);
static_assert(minidayz::stage1::kGroupCount==minidayz::stage1::kNamedGroups);