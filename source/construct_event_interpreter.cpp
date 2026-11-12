#include "minidayz/construct_event_interpreter.hpp"

#include <cstddef>
#include <cstdint>

namespace minidayz::construct {
namespace {

constexpr std::size_t kMaxEventDepth = 64;
constexpr std::uint32_t kNoParent = 0xffffffffu;

bool execute_node(std::size_t node_index, int inherited_loop_index,
                  bool previous_sibling_matched, const EventCallbacks& callbacks,
                  std::size_t recursion_depth, bool& node_matched) {
    node_matched = false;
    if (recursion_depth > kMaxEventDepth ||
        node_index >= stage1::kLocationEventSourceCount) return false;

    const auto& node = stage1::kLocationEventSource[node_index];
    if (node.disabled) return true;

    int repeat_count = 1;
    bool repeated = false;
    for (std::size_t i = 0; i < node.condition_count; ++i) {
        const auto& condition =
            stage1::kLocationEventConditionSource[node.first_condition + i];

        // Construct's Else condition depends on the immediately preceding
        // sibling event's condition result, not on whether its actions ran.
        if (condition.object_index == -1 && condition.command_id == 70) {
            if (previous_sibling_matched) return true;
            continue;
        }

        if (!callbacks.evaluate_condition) return false;
        bool condition_matched = false;
        int requested_repeat_count = 1;
        if (!callbacks.evaluate_condition(callbacks.user, condition,
                                          inherited_loop_index,
                                          previous_sibling_matched,
                                          condition_matched,
                                          requested_repeat_count)) return false;
        if (!condition_matched) return true;

        if (condition.object_index == -1 && condition.command_id == 123) {
            repeated = true;
            repeat_count = requested_repeat_count;
            if (repeat_count <= 0) return true;
        }
    }

    node_matched = true;
    const int iterations = repeated ? repeat_count : 1;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        const int loop_index = repeated ? iteration : inherited_loop_index;

        for (std::size_t i = 0; i < node.action_count; ++i) {
            if (!callbacks.execute_action) return false;
            const auto& action =
                stage1::kLocationEventActionSource[node.first_action + i];
            if (!callbacks.execute_action(callbacks.user, action, loop_index))
                return false;
        }

        bool previous_child_matched = true;
        for (std::size_t child = 0; child < stage1::kLocationEventSourceCount; ++child) {
            if (stage1::kLocationEventSource[child].parent_index != node_index)
                continue;
            bool child_matched = false;
            if (!execute_node(child, loop_index, previous_child_matched, callbacks,
                              recursion_depth + 1, child_matched)) return false;
            previous_child_matched = child_matched;
        }
    }
    return true;
}

} // namespace

bool execute_location_event_branch(std::uint64_t branch_uid,
                                   const EventCallbacks& callbacks,
                                   bool& branch_matched) {
    branch_matched = false;
    std::size_t branch_index = stage1::kLocationEventSourceCount;
    for (std::size_t i = 0; i < stage1::kLocationEventSourceCount; ++i) {
        const auto& event = stage1::kLocationEventSource[i];
        if (event.branch_uid == branch_uid && event.depth == 1 &&
            event.parent_index != kNoParent) {
            branch_index = i;
            break;
        }
    }
    if (branch_index == stage1::kLocationEventSourceCount) return false;
    return execute_node(branch_index, 0, true, callbacks, 0, branch_matched);
}

} // namespace minidayz::construct
