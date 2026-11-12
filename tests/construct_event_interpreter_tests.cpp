#include "minidayz/construct_event_interpreter.hpp"
#include "minidayz/generated/location_event_source.hpp"
#include "minidayz/world.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>

using namespace minidayz;

namespace {
struct TestState {
    std::size_t array_writes{};
    std::size_t array_sizes{};
    int repeat_indices_seen{};
};

bool evaluate_condition(void*, const stage1::LocationEventConditionSource& condition,
                        int, bool, bool& matched, int& repeat_count) {
    matched = true;
    if (condition.object_index == -1 && condition.command_id == 123)
        repeat_count = 16;
    return true;
}

bool execute_action(void* user, const stage1::LocationEventActionSource& action,
                    int loop_index) {
    auto& state = *static_cast<TestState*>(user);
    if (loop_index >= 0 && loop_index < 16)
        state.repeat_indices_seen |= (1 << loop_index);
    if (action.object_index == 525 && action.command_id == 199) {
        ++state.array_writes;
    }
    if (action.object_index == 525 && action.command_id == 210)
        ++state.array_sizes;
    return true;
}

std::uint64_t branch_for_code(std::uint16_t code) {
    for (const auto& entry : stage1::kLocationTemplateSource) {
        for (std::uint8_t i = 0; i < entry.tile_code_count; ++i)
            if (entry.tile_codes[i] == code) return entry.event_uid;
    }
    return 0;
}
}

int main() {
    construct::EvaluationContext parameter_context{};
    construct::Value parameter_value;
    const auto repeat_branch = branch_for_code(52);
    const stage1::LocationEventConditionSource* repeat_condition = nullptr;
    for (std::size_t i = 0; i < stage1::kLocationEventConditionSourceCount; ++i) {
        const auto& condition = stage1::kLocationEventConditionSource[i];
        if (condition.branch_uid == repeat_branch &&
            condition.object_index == -1 && condition.command_id == 123) {
            repeat_condition = &condition;
            break;
        }
    }
    assert(repeat_condition && repeat_condition->parameter_count == 1);
    assert(construct::evaluate_location_event_parameter(
        stage1::kLocationEventParameterSource[repeat_condition->first_parameter],
        parameter_context, parameter_value));
    assert(parameter_value.as_number() == 16);

    const auto simple_branch = branch_for_code(50);
    const stage1::LocationEventActionSource* create_action = nullptr;
    for (std::size_t i = 0; i < stage1::kLocationEventActionSourceCount; ++i) {
        const auto& action = stage1::kLocationEventActionSource[i];
        if (action.branch_uid == simple_branch &&
            action.object_index == -1 && action.command_id == 94) {
            create_action = &action;
            break;
        }
    }
    assert(create_action && create_action->parameter_count == 4);
    assert(construct::evaluate_location_event_parameter(
        stage1::kLocationEventParameterSource[create_action->first_parameter + 1],
        parameter_context, parameter_value));
    assert(parameter_value.as_string() == "zed_resp");
    construct::EventCallbacks callbacks{};
    callbacks.evaluate_condition = evaluate_condition;
    callbacks.execute_action = execute_action;

    TestState simple_state;
    callbacks.user = &simple_state;
    bool matched = false;
    assert(construct::execute_location_event_branch(branch_for_code(50), callbacks, matched));
    assert(matched);
    assert(simple_state.array_writes == 45);
    assert(simple_state.array_sizes == 1);

    // Tile code 52 contains the source Repeat(16) plant-placement event.
    TestState repeat_state;
    callbacks.user = &repeat_state;
    assert(construct::execute_location_event_branch(branch_for_code(52), callbacks, matched));
    assert(matched);
    assert(repeat_state.array_writes == 72);
    assert(repeat_state.array_sizes == 1);
    assert(repeat_state.repeat_indices_seen == 0xffff);
    return 0;
}
