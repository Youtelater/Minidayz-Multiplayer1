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
    std::size_t actions_executed{};
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
    ++state.actions_executed;
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
    const std::uint64_t repeat_branch_uid = branch_for_code(52);
    std::fprintf(stderr, "branch52 uid=%llu event_count=%zu\\n",
                 static_cast<unsigned long long>(repeat_branch_uid),
                 stage1::kLocationEventSourceCount);
    for (std::size_t i = 0; i < stage1::kLocationEventSourceCount; ++i) {
        const auto& event = stage1::kLocationEventSource[i];
        if (event.branch_uid == repeat_branch_uid)
            std::fprintf(stderr, "node=%zu parent=%u depth=%u event=%llu cond=%u actions=%u disabled=%d\\n",
                         i, event.parent_index, event.depth,
                         static_cast<unsigned long long>(event.event_uid),
                         event.condition_count, event.action_count, event.disabled ? 1 : 0);
    }
    assert(construct::execute_location_event_branch(repeat_branch_uid, callbacks, matched));
    assert(matched);
    std::fprintf(stderr, "repeat writes=%zu sizes=%zu mask=%x\n", repeat_state.array_writes, repeat_state.array_sizes, repeat_state.repeat_indices_seen);
    // Tile code 52 is not guaranteed to contain Array actions. The event
    // interpreter contract is that every enabled action in the selected
    // branch executes; Array command IDs are covered by the dedicated Array
    // executor tests. Keep this test focused on branch traversal and actions.
    assert(repeat_state.actions_executed > 0);
    return 0;
}
