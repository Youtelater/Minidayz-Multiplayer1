#!/usr/bin/env python3
"""Extract the original generate_location dispatch branches into a C++ source index.

The emitted records preserve Construct event UIDs, marker-code conditions, and
counts of the original Array write / System create actions. This is an index of
the source event tree, not an execution of those actions; command semantics
must be decoded before templates can be instantiated in the native world.
"""
import json
from pathlib import Path

SOURCE = Path("data/stage1/construct_project_parsed.json")
OUTPUT = Path("include/minidayz/generated/location_template_source.hpp")
FUNCTION_NAME = "generate_location"
FUNCTION_UID = 5017992314331459

def walk_events(items):
    for item in items:
        if not isinstance(item, list) or not item:
            continue
        if item[0] == 0:
            yield item
            yield from walk_events(item[7] if len(item) > 7 and isinstance(item[7], list) else [])

def count_branch(branch):
    nodes = writes = creates = 0
    stack = [branch]
    while stack:
        node = stack.pop()
        if not isinstance(node, list) or len(node) < 7 or node[0] != 0:
            continue
        nodes += 1
        for action in node[6] if isinstance(node[6], list) else []:
            if not isinstance(action, list) or len(action) < 2:
                continue
            if action[0] == 525 and action[1] == 199:
                writes += 1
            if action[0] == -1 and action[1] == 94:
                creates += 1
        stack.extend(node[7] if len(node) > 7 and isinstance(node[7], list) else [])
    return nodes, writes, creates

def collect_array_writes(branch):
    """Preserve every original Array write's expression tree for native execution."""
    writes = []
    stack = [branch]
    while stack:
        node = stack.pop()
        if not isinstance(node, list) or len(node) < 7 or node[0] != 0:
            continue
        actions = node[6] if isinstance(node[6], list) else []
        for action in actions:
            if not isinstance(action, list) or len(action) < 6:
                continue
            if action[0] != 525 or action[1] != 199:
                continue
            params = action[5]
            if len(params) < 3 or any(not isinstance(p, list) or len(p) < 2 for p in params[:3]):
                raise SystemExit(f"Malformed Array write action {action[3]}")
            writes.append((branch[4], node[4], action[3],
                           *(json.dumps(p[1], separators=(",", ":")) for p in params[:3])))
        # Reverse before pushing so traversal retains Construct event order.
        children = node[7] if len(node) > 7 and isinstance(node[7], list) else []
        stack.extend(reversed(children))
    return writes

def collect_expression_inventory(rows):
    opcode_counts = {}
    function_counts = {}

    def walk(node):
        if not isinstance(node, list):
            return
        if node and isinstance(node[0], int):
            opcode = node[0]
            opcode_counts[opcode] = opcode_counts.get(opcode, 0) + 1
            if opcode == 19 and len(node) > 1 and isinstance(node[1], int):
                function_id = node[1]
                function_counts[function_id] = function_counts.get(function_id, 0) + 1
        for child in node:
            walk(child)

    for row in rows:
        for expression in row[3:]:
            walk(json.loads(expression))
    return sorted(opcode_counts.items()), sorted(function_counts.items())


def collect_array_operations(branch):
    """Preserve Array Set-at-XY and Set-size calls in one ordered stream."""
    operations = []
    stack = [branch]
    while stack:
        node = stack.pop()
        if not isinstance(node, list) or len(node) < 7 or node[0] != 0:
            continue
        for action in node[6] if isinstance(node[6], list) else []:
            if not isinstance(action, list) or len(action) < 6:
                continue
            if action[0] != 525 or action[1] not in (199, 210):
                continue
            params = action[5]
            if len(params) != 3 or any(not isinstance(p, list) or len(p) < 2 for p in params):
                raise SystemExit(f"Malformed Array operation {action[3]}")
            operations.append((branch[4], node[4], action[3], action[1],
                               *(json.dumps(p[1], separators=(",", ":")) for p in params)))
        children = node[7] if len(node) > 7 and isinstance(node[7], list) else []
        stack.extend(reversed(children))
    return operations

def cpp_string(value):
    return json.dumps(value)

def main():
    root = json.loads(SOURCE.read_text(encoding="utf-8"))
    project = root["project"]
    sheets = {name: entries for name, entries in project[6]}
    candidates = [n for n in walk_events(sheets["Game_events"])
                  if n[4] == FUNCTION_UID]
    if len(candidates) != 1:
        raise SystemExit(f"Expected one {FUNCTION_NAME} function event, found {len(candidates)}")
    function = candidates[0]
    event_roots = [x for x in function[7] if isinstance(x, list) and x and x[0] == 0]
    if len(event_roots) != 1:
        raise SystemExit(f"Expected one root event in {FUNCTION_NAME}, found {len(event_roots)}")
    branches = event_roots[0][7]
    rows = []
    spawn_rows = []
    array_write_rows = []
    array_operation_rows = []
    for branch in branches:
        if not isinstance(branch, list) or not branch or branch[0] != 0:
            continue
        codes = []
        for condition in branch[5] if isinstance(branch[5], list) else []:
            params = condition[9] if len(condition) > 9 and isinstance(condition[9], list) else []
            last = params[-1] if params else None
            if (condition[0] == 514 and condition[1] == 71 and
                isinstance(last, list) and len(last) > 1 and last[0] == 7 and
                isinstance(last[1], list) and len(last[1]) > 1 and last[1][0] == 0):
                codes.append(int(last[1][1]))
        if not codes:
            raise SystemExit(f"Location branch UID {branch[4]} has no decoded tile-code condition")
        nodes, writes, creates = count_branch(branch)
        rows.append((branch[4], codes, nodes, writes, creates))
        array_write_rows.extend(collect_array_writes(branch))
        array_operation_rows.extend(collect_array_operations(branch))

        def hotspot_offset(expression, variable):
            if expression == [23, variable]:
                return 0
            if (isinstance(expression, list) and len(expression) == 3 and
                expression[0] == 4 and expression[1] == [23, variable] and
                isinstance(expression[2], list) and expression[2][0] == 0):
                return int(expression[2][1])
            raise SystemExit(
                f"Unsupported {variable} location spawn expression in branch {branch[4]}: {expression!r}")

        stack = [branch]
        while stack:
            node = stack.pop()
            if not isinstance(node, list) or len(node) < 8 or node[0] != 0:
                continue
            for action in node[6] if isinstance(node[6], list) else []:
                if not isinstance(action, list) or len(action) < 6 or action[0] != -1 or action[1] != 94:
                    continue
                params = action[5]
                if len(params) < 4:
                    raise SystemExit(f"Malformed System Create action in branch {branch[4]}")
                object_index = int(params[0][1])
                layer_name = str(params[1][1][1])
                dx = hotspot_offset(params[2][1], "hotspotX")
                dy = hotspot_offset(params[3][1], "hotspotY")
                for code in codes:
                    spawn_rows.append((branch[4], int(action[3]), code,
                                       object_index, layer_name, dx, dy))
            # Preserve Construct's authored event order; generated spawn rows are
            # appended to the map in this order, so reversing siblings changes
            # overlap/draw ordering for objects created by nested events.
            children = node[7] if len(node) > 7 and isinstance(node[7], list) else []
            stack.extend(reversed(children))
    lines = ["#pragma once", "#include <cstddef>", "#include <cstdint>",
             "namespace minidayz::stage1 {",
             "struct LocationTemplateSource { std::uint64_t event_uid; std::uint16_t tile_codes[25]; std::uint8_t tile_code_count; std::uint16_t event_nodes; std::uint16_t array_write_actions; std::uint8_t system_create_actions; };",
             f"inline constexpr std::size_t kLocationTemplateSourceCount = {len(rows)};",
             "inline constexpr LocationTemplateSource kLocationTemplateSource[] = {"]
    for uid, codes, nodes, writes, creates in rows:
        if len(codes) > 25:
            raise SystemExit(f"Branch {uid} has {len(codes)} tile codes; descriptor capacity is 25")
        padded = codes + [0] * (25 - len(codes))
        lines.append(f"    {{{uid}ULL, {{{', '.join(map(str, padded))}}}, {len(codes)}, {nodes}, {writes}, {creates}}},")
    lines += ["};",
              "struct LocationArrayWriteSource { std::uint64_t branch_uid, event_uid, action_uid; const char* array_expression; const char* index_expression; const char* value_expression; };",
              f"inline constexpr std::size_t kLocationArrayWriteSourceCount = {len(array_write_rows)};",
              "inline constexpr LocationArrayWriteSource kLocationArrayWriteSource[] = {"]
    for branch_uid, event_uid, action_uid, array_expr, index_expr, value_expr in array_write_rows:
        lines.append(f"    {{{branch_uid}ULL, {event_uid}ULL, {action_uid}ULL, {cpp_string(array_expr)}, {cpp_string(index_expr)}, {cpp_string(value_expr)}}},")
    opcode_counts, function_counts = collect_expression_inventory(array_write_rows)
    lines += ["};",
              "struct LocationExpressionOpcodeCount { std::uint16_t opcode; std::uint32_t count; };",
              f"inline constexpr std::size_t kLocationExpressionOpcodeCount = {len(opcode_counts)};",
              "inline constexpr LocationExpressionOpcodeCount kLocationExpressionOpcodeCounts[] = {"]
    for opcode, count in opcode_counts:
        lines.append(f"    {{{opcode}, {count}}},")
    lines += ["};",
              "struct LocationExpressionFunctionCount { std::uint16_t function_id; std::uint32_t count; };",
              f"inline constexpr std::size_t kLocationExpressionFunctionCount = {len(function_counts)};",
              "inline constexpr LocationExpressionFunctionCount kLocationExpressionFunctionCounts[] = {"]
    for function_id, count in function_counts:
        lines.append(f"    {{{function_id}, {count}}},")
    lines += ["};", "} // namespace minidayz::stage1", ""]
    # Preserve the full event tree as executable input for the native
    # interpreter. The older location-template table intentionally contains
    # only dispatch/spawn summaries and cannot represent nested conditions,
    # Else siblings, or Repeat loops.
    event_rows = []
    condition_rows = []
    action_rows = []
    def collect_event_tree(root_event):
        stack = [(root_event, 0xffffffff, 0, 0)]
        while stack:
            node, parent_index, depth, branch_uid = stack.pop()
            if not isinstance(node, list) or len(node) < 7 or node[0] != 0:
                continue
            if depth == 1:
                branch_uid = node[4]
            node_index = len(event_rows)
            first_condition = len(condition_rows)
            for condition in node[5] if isinstance(node[5], list) else []:
                if not isinstance(condition, list) or len(condition) < 2:
                    continue
                condition_rows.append((branch_uid, node[4], int(condition[0]),
                                       int(condition[1]), json.dumps(condition, separators=(",", ":"))))
            first_action = len(action_rows)
            for action in node[6] if isinstance(node[6], list) else []:
                if not isinstance(action, list) or len(action) < 2:
                    continue
                action_rows.append((branch_uid, node[4], int(action[0]), int(action[1]),
                                    json.dumps(action, separators=(",", ":"))))
            event_rows.append((branch_uid, node[4], parent_index, depth,
                               first_condition, len(condition_rows) - first_condition,
                               first_action, len(action_rows) - first_action,
                               bool(node[2]) if len(node) > 2 else False))
            children = node[7] if len(node) > 7 and isinstance(node[7], list) else []
            stack.extend((child, node_index, depth + 1, branch_uid)
                         for child in reversed(children) if isinstance(child, list))
    collect_event_tree(event_roots[0])

    event_lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::stage1 {",
        "struct LocationEventSource { std::uint64_t branch_uid, event_uid; std::uint32_t parent_index, first_condition, condition_count, first_action, action_count; std::uint16_t depth; bool disabled; };",
        f"inline constexpr std::size_t kLocationEventSourceCount = {len(event_rows)};",
        "inline constexpr LocationEventSource kLocationEventSource[] = {"
    ]
    for branch_uid, event_uid, parent, depth, first_condition, condition_count, first_action, action_count, disabled in event_rows:
        event_lines.append(
            f"    {{{branch_uid}ULL, {event_uid}ULL, {parent}u, {first_condition}u, {condition_count}u, {first_action}u, {action_count}u, {depth}, {str(disabled).lower()}}},")
    event_lines += [
        "};",
        "struct LocationEventConditionSource { std::uint64_t branch_uid, event_uid; std::int16_t object_index; std::uint16_t command_id; const char* raw_call; };",
        f"inline constexpr std::size_t kLocationEventConditionSourceCount = {len(condition_rows)};",
        "inline constexpr LocationEventConditionSource kLocationEventConditionSource[] = {"
    ]
    for branch_uid, event_uid, object_index, command_id, raw_call in condition_rows:
        event_lines.append(
            f"    {{{branch_uid}ULL, {event_uid}ULL, {object_index}, {command_id}, {cpp_string(raw_call)}}},")
    event_lines += [
        "};",
        "struct LocationEventActionSource { std::uint64_t branch_uid, event_uid; std::int16_t object_index; std::uint16_t command_id; const char* raw_call; };",
        f"inline constexpr std::size_t kLocationEventActionSourceCount = {len(action_rows)};",
        "inline constexpr LocationEventActionSource kLocationEventActionSource[] = {"
    ]
    for branch_uid, event_uid, object_index, command_id, raw_call in action_rows:
        event_lines.append(
            f"    {{{branch_uid}ULL, {event_uid}ULL, {object_index}, {command_id}, {cpp_string(raw_call)}}},")
    event_lines += ["};", "} // namespace minidayz::stage1", ""]
    event_output = OUTPUT.with_name("location_event_source.hpp")
    event_output.write_text("\n".join(event_lines), encoding="utf-8")

    operation_lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::stage1 {",
        "struct LocationArrayOperationSource { std::uint64_t branch_uid, event_uid, action_uid; std::uint16_t action_id; const char* parameter_0; const char* parameter_1; const char* parameter_2; };",
        f"inline constexpr std::size_t kLocationArrayOperationSourceCount = {len(array_operation_rows)};",
        "inline constexpr LocationArrayOperationSource kLocationArrayOperationSource[] = {"
    ]
    for branch_uid, event_uid, action_uid, action_id, p0, p1, p2 in array_operation_rows:
        operation_lines.append(
            f"    {{{branch_uid}ULL, {event_uid}ULL, {action_uid}ULL, {action_id}, {cpp_string(p0)}, {cpp_string(p1)}, {cpp_string(p2)}}},")
    operation_lines += ["};", "} // namespace minidayz::stage1", ""]
    operation_output = OUTPUT.with_name("location_array_operation_source.hpp")
    operation_output.write_text("\n".join(operation_lines), encoding="utf-8")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    spawn_output = OUTPUT.with_name("location_spawn_source.hpp")
    spawn_lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::stage1 {",
        "struct LocationSpawnSource { std::uint64_t event_uid, action_uid; std::uint16_t tile_code, object; const char* layer_name; std::int16_t offset_x, offset_y; };",
        f"inline constexpr std::size_t kLocationSpawnSourceCount = {len(spawn_rows)};",
        "inline constexpr LocationSpawnSource kLocationSpawnSource[] = {"
    ]
    for event_uid, action_uid, code, obj, layer, dx, dy in spawn_rows:
        spawn_lines.append(
            f'    {{{event_uid}ULL, {action_uid}ULL, {code}, {obj}, "{layer}", {dx}, {dy}}},')
    spawn_lines += ["};", "} // namespace minidayz::stage1", ""]
    spawn_output.write_text("\n".join(spawn_lines), encoding="utf-8")
    print(f"Extracted {len(spawn_rows)} original System Create actions -> {spawn_output}")
    print(f"Indexed {len(rows)} original generate_location branches -> {OUTPUT}")
    print(f"Indexed {len(event_rows)} event nodes, {len(condition_rows)} conditions, and {len(action_rows)} actions -> {event_output}")
    print(f"Extracted {len(array_operation_rows)} ordered Array operations -> {operation_output}")
    print(f"Preserved {len(array_write_rows)} Array writes")

if __name__ == "__main__":
    main()
