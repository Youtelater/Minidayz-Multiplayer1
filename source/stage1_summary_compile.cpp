// Compile-only Stage 1 metadata assertions. Kept under source/ per project convention.
#include "minidayz/stage1_summary.hpp"
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
