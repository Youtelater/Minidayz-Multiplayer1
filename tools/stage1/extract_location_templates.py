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
        if not isinstance(node, list) or len(node) < 8 or node[0] != 0:
            continue
        nodes += 1
        for action in node[6] if isinstance(node[6], list) else []:
            if not isinstance(action, list) or len(action) < 2:
                continue
            if action[0] == 525 and action[1] == 199:
                writes += 1
            if action[0] == -1 and action[1] == 94:
                creates += 1
        stack.extend(node[7] if isinstance(node[7], list) else [])
    return nodes, writes, creates

def collect_array_writes(branch):
    """Preserve every original Array write's expression tree for native execution."""
    writes = []
    stack = [branch]
    while stack:
        node = stack.pop()
        if not isinstance(node, list) or len(node) < 8 or node[0] != 0:
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
            writes.append((node[4], action[3],
                           *(json.dumps(p[1], separators=(",", ":")) for p in params[:3])))
        # Reverse before pushing so traversal retains Construct event order.
        children = node[7] if isinstance(node[7], list) else []
        stack.extend(reversed(children))
    return writes

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
            stack.extend(node[7] if isinstance(node[7], list) else [])
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
              "struct LocationArrayWriteSource { std::uint64_t event_uid, action_uid; const char* array_expression; const char* index_expression; const char* value_expression; };",
              f"inline constexpr std::size_t kLocationArrayWriteSourceCount = {len(array_write_rows)};",
              "inline constexpr LocationArrayWriteSource kLocationArrayWriteSource[] = {"]
    for event_uid, action_uid, array_expr, index_expr, value_expr in array_write_rows:
        lines.append(f"    {{{event_uid}ULL, {action_uid}ULL, {cpp_string(array_expr)}, {cpp_string(index_expr)}, {cpp_string(value_expr)}}},")
    lines += ["};", "} // namespace minidayz::stage1", ""]
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

if __name__ == "__main__":
    main()
