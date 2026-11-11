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
            if action[0] == 518 and action[1] == 199:
                writes += 1
            if action[0] == -1 and action[1] == 94:
                creates += 1
        stack.extend(node[7] if isinstance(node[7], list) else [])
    return nodes, writes, creates

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
    if len(rows) != 62:
        raise SystemExit(f"Expected 62 original location branches, found {len(rows)}")
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
    lines += ["};", "} // namespace minidayz::stage1", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Indexed {len(rows)} original generate_location branches -> {OUTPUT}")

if __name__ == "__main__":
    main()
