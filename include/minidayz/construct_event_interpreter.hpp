#pragma once

#include "minidayz/construct_expression.hpp"

#include <cstdint>

namespace minidayz::stage1 {
struct LocationEventConditionSource;
struct LocationEventActionSource;
struct LocationEventParameterSource;
}

namespace minidayz::construct {

using EventConditionEvaluator = bool (*)(void* user,
    const stage1::LocationEventConditionSource& condition, int loop_index,
    bool previous_sibling_matched, bool& matched, int& repeat_count);
using EventActionExecutor = bool (*)(void* user,
    const stage1::LocationEventActionSource& action, int loop_index);

struct EventCallbacks {
    void* user{};
    EventConditionEvaluator evaluate_condition{};
    EventActionExecutor execute_action{};
};

// Executes one source generate_location dispatch branch. Conditions and
// actions are delegated to source-specific callbacks; this layer preserves
// Construct's event order, nested subevents, Else sibling state and Repeat
// loop-index behavior.
bool evaluate_location_event_parameter(
    const stage1::LocationEventParameterSource& parameter,
    EvaluationContext& context, Value& result);

bool execute_location_event_branch(std::uint64_t branch_uid,
                                   const EventCallbacks& callbacks,
                                   bool& branch_matched);

} // namespace minidayz::construct
