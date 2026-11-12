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
    if (action.object_index == 525 && action.command_id == 199) {
        ++state.array_writes;
        if (loop_index >= 0 && loop_index < 12)
            state.repeat_indices_seen |= (1 << loop_index);
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

    TestState repeat_state;
    callbacks.user = &repeat_state;
    assert(construct::execute_location_event_branch(branch_for_code(52), callbacks, matched));
    assert(matched);
    assert(repeat_state.array_writes == 72);
    assert(repeat_state.array_sizes == 1);
    assert(repeat_state.repeat_indices_seen == 0xffff);
    return 0;
}
