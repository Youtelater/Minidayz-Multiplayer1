#!/usr/bin/env python3
"""Extract every original Construct Spawn_NPC function call and its arguments."""
import json
import re
from collections import Counter
from pathlib import Path

SOURCE = Path("data/stage1/construct_project_parsed.json")
OUTPUT = Path("include/minidayz/generated/npc_spawn_call_source.hpp")
FUNCTION_OBJECT = 189
ON_FUNCTION = 40
CALL_FUNCTION = 69
FUNCTION_NAME = "Spawn_NPC"

def cpp_string(value):
    return json.dumps(value, ensure_ascii=True)

def is_function_name(parameter):
    return (isinstance(parameter, list) and len(parameter) > 1 and
            parameter[1] == [2, FUNCTION_NAME])

def walk_events(items, sheet_name):
    for event in items:
        if not isinstance(event, list) or not event or event[0] != 0:
            continue
        yield sheet_name, event
        children = event[7] if len(event) > 7 and isinstance(event[7], list) else []
        yield from walk_events(children, sheet_name)

def main():
    source_text = SOURCE.read_text(encoding="utf-8")
    # Match extract_construct.load(): the original Construct export contains
    # whitespace between unary minus and numeric literals, which JSON rejects.
    source_text = re.sub(r"(?<![A-Za-z0-9_.])-\\s+(?=\\d)", "-", source_text)
    root = json.loads(source_text)
    project = root["project"]
    calls = []
    definitions = []
    for sheet_name, entries in project[6]:
        for current_sheet, event in walk_events(entries, sheet_name):
            conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
            for condition in conditions:
                if (isinstance(condition, list) and len(condition) > 9 and
                    condition[0] == FUNCTION_OBJECT and condition[1] == ON_FUNCTION and
                    condition[9] and is_function_name(condition[9][0])):
                    definitions.append((current_sheet, event[4]))
            actions = event[6] if len(event) > 6 and isinstance(event[6], list) else []
            for action in actions:
                if not (isinstance(action, list) and len(action) > 5 and
                        action[0] == FUNCTION_OBJECT and action[1] == CALL_FUNCTION):
                    continue
                parameters = action[5]
                if not (isinstance(parameters, list) and len(parameters) == 2 and
                        is_function_name(parameters[0])):
                    continue
                argument_list = parameters[1]
                if not (isinstance(argument_list, list) and argument_list and
                        argument_list[0] == 13):
                    raise SystemExit(f"Malformed {FUNCTION_NAME} call action {action[3]}")
                arguments = []
                for item in argument_list[1:]:
                    if not (isinstance(item, list) and len(item) > 1 and item[0] == 7):
                        raise SystemExit(f"Malformed {FUNCTION_NAME} argument in action {action[3]}")
                    arguments.append(json.dumps(item[1], separators=(",", ":"), ensure_ascii=True))
                if len(arguments) not in (3, 4):
                    raise SystemExit(f"Unexpected {FUNCTION_NAME} argument count {len(arguments)} in action {action[3]}")
                calls.append((current_sheet, event[4], action[3], arguments))

    if len(definitions) != 1:
        raise SystemExit(f"Expected one {FUNCTION_NAME} definition, found {len(definitions)}")
    if len(calls) != 162:
        raise SystemExit(f"Expected 162 original {FUNCTION_NAME} callsites, found {len(calls)}")

    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::stage1 {",
        "struct NpcSpawnCallSource { const char* event_sheet; std::uint64_t event_uid; std::uint64_t action_uid; std::uint8_t argument_count; const char* arguments[4]; };",
        f"inline constexpr std::uint64_t kNpcFunctionDefinitionEventUid = {definitions[0][1]}ULL;",
        f"inline constexpr std::size_t kNpcSpawnCallSourceCount = {len(calls)};",
        "inline constexpr NpcSpawnCallSource kNpcSpawnCallSource[] = {",
    ]
    for sheet, event_uid, action_uid, arguments in calls:
        padded = arguments + ["nullptr"] * (4 - len(arguments))
        encoded = [value if value == "nullptr" else cpp_string(value) for value in padded]
        line = ("    {" + cpp_string(sheet) + f", {event_uid}ULL, {action_uid}ULL, "
                + f"{len(arguments)}, {{" + ", ".join(encoded) + "}},")
        lines.append(line)
    lines += ["};", "} // namespace minidayz::stage1", ""]
    # Preserve the complete original Spawn_NPC event tree as native source data.
    # The callsite table above alone is not executable: it omits the function body.
    function_event_rows = []
    function_condition_rows = []
    function_action_rows = []
    function_parameter_rows = []
    # Re-find the definition node without depending on event traversal indexes.
    function_node = None
    for sheet_name, entries in project[6]:
        for _, event in walk_events(entries, sheet_name):
            conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
            if event[4] == definitions[0][1] and any(
                    isinstance(c, list) and len(c) > 9 and c[0] == FUNCTION_OBJECT
                    and c[1] == ON_FUNCTION and c[9]
                    and is_function_name(c[9][0]) for c in conditions):
                function_node = event
                break
        if function_node is not None:
            break
    if function_node is None:
        raise SystemExit("Spawn_NPC definition disappeared during extraction")

    stack = [(function_node, 0xffffffff, 0)]
    while stack:
        node, parent_index, depth = stack.pop()
        if not isinstance(node, list) or len(node) < 7 or node[0] != 0:
            continue
        node_index = len(function_event_rows)
        first_condition = len(function_condition_rows)
        for condition in node[5] if isinstance(node[5], list) else []:
            if not isinstance(condition, list) or len(condition) < 2:
                continue
            first_parameter = len(function_parameter_rows)
            params = condition[9] if len(condition) > 9 and isinstance(condition[9], list) else []
            for parameter in params:
                if isinstance(parameter, list) and len(parameter) > 1:
                    function_parameter_rows.append((int(parameter[0]),
                        json.dumps(parameter[1], separators=(",", ":"))))
            function_condition_rows.append((int(condition[0]), int(condition[1]),
                first_parameter, len(function_parameter_rows) - first_parameter,
                json.dumps(condition, separators=(",", ":"))))
        first_action = len(function_action_rows)
        for action in node[6] if isinstance(node[6], list) else []:
            if not isinstance(action, list) or len(action) < 2:
                continue
            first_parameter = len(function_parameter_rows)
            params = action[5] if len(action) > 5 and isinstance(action[5], list) else []
            for parameter in params:
                if isinstance(parameter, list) and len(parameter) > 1:
                    function_parameter_rows.append((int(parameter[0]),
                        json.dumps(parameter[1], separators=(",", ":"))))
            function_action_rows.append((int(action[0]), int(action[1]),
                first_parameter, len(function_parameter_rows) - first_parameter,
                json.dumps(action, separators=(",", ":"))))
        function_event_rows.append((node[4], parent_index, depth,
            first_condition, len(function_condition_rows) - first_condition,
            first_action, len(function_action_rows) - first_action,
            bool(node[2]) if len(node) > 2 else False))
        children = node[7] if len(node) > 7 and isinstance(node[7], list) else []
        stack.extend((child, node_index, depth + 1)
                     for child in reversed(children) if isinstance(child, list))

    function_lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::stage1 {",
        "struct NpcFunctionEventSource { std::uint64_t event_uid; std::uint32_t parent_index, first_condition, condition_count, first_action, action_count; std::uint16_t depth; bool disabled; };",
        f"inline constexpr std::uint64_t kNpcSpawnFunctionEventUid = {definitions[0][1]}ULL;",
        f"inline constexpr std::size_t kNpcFunctionEventSourceCount = {len(function_event_rows)};",
        "inline constexpr NpcFunctionEventSource kNpcFunctionEventSource[] = {"
    ]
    for row in function_event_rows:
        uid, parent, depth, fc, cc, fa, ac, disabled = row
        function_lines.append(f"    {{{uid}ULL, {parent}u, {fc}u, {cc}u, {fa}u, {ac}u, {depth}, {str(disabled).lower()}}},")
    function_lines += [
        "};",
        "struct NpcFunctionConditionSource { std::int16_t object_index; std::uint16_t command_id; std::uint32_t first_parameter; std::uint16_t parameter_count; const char* raw_call; };",
        f"inline constexpr std::size_t kNpcFunctionConditionSourceCount = {len(function_condition_rows)};",
        "inline constexpr NpcFunctionConditionSource kNpcFunctionConditionSource[] = {"
    ]
    for obj, command, first, count, raw in function_condition_rows:
        function_lines.append(f"    {{{obj}, {command}, {first}u, {count}, {cpp_string(raw)}}},")
    function_lines += [
        "};",
        "struct NpcFunctionActionSource { std::int16_t object_index; std::uint16_t command_id; std::uint32_t first_parameter; std::uint16_t parameter_count; const char* raw_call; };",
        f"inline constexpr std::size_t kNpcFunctionActionSourceCount = {len(function_action_rows)};",
        "inline constexpr NpcFunctionActionSource kNpcFunctionActionSource[] = {"
    ]
    for obj, command, first, count, raw in function_action_rows:
        function_lines.append(f"    {{{obj}, {command}, {first}u, {count}, {cpp_string(raw)}}},")
    function_lines += [
        "};",
        "struct NpcFunctionParameterSource { std::uint16_t type; const char* raw_value; };",
        f"inline constexpr std::size_t kNpcFunctionParameterSourceCount = {len(function_parameter_rows)};",
        "inline constexpr NpcFunctionParameterSource kNpcFunctionParameterSource[] = {"
    ]
    for parameter_type, raw in function_parameter_rows:
        function_lines.append(f"    {{{parameter_type}, {cpp_string(raw)}}},")
    function_lines += ["};", "} // namespace minidayz::stage1", ""]
    function_output = OUTPUT.with_name("npc_spawn_function_source.hpp")
    function_output.parent.mkdir(parents=True, exist_ok=True)
    function_output.write_text("\n".join(function_lines), encoding="utf-8")
    if len(function_event_rows) != 399:
        raise SystemExit(f"Expected 399 Spawn_NPC event nodes, found {len(function_event_rows)}")

    game_event_rows = []
    game_event_action_rows = []
    for sheet_name, entries in project[6]:
        if sheet_name != "Game_events":
            continue
        for _, event in walk_events(entries, sheet_name):
            actions = event[6] if len(event) > 6 and isinstance(event[6], list) else []
            first_action = len(game_event_action_rows)
            for action in actions:
                if not isinstance(action, list) or len(action) < 4:
                    continue
                game_event_action_rows.append((int(action[3]), int(action[0]), int(action[1])))
            game_event_rows.append((int(event[4]), first_action,
                                    len(game_event_action_rows) - first_action))
    if not game_event_rows or not game_event_action_rows:
        raise SystemExit("Game_events source extraction produced no events or actions")
    game_event_lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::stage1 {",
        "struct GameEventSource { std::uint64_t event_uid; std::uint32_t first_action, action_count; };",
        f"inline constexpr std::size_t kGameEventSourceCount = {len(game_event_rows)};",
        "inline constexpr GameEventSource kGameEventSource[] = {",
    ]
    for uid, first, count in game_event_rows:
        game_event_lines.append(f"    {{{uid}ULL, {first}u, {count}u}},")
    game_event_lines += [
        "};",
        "struct GameEventActionSource { std::uint64_t action_uid; std::int16_t object_index; std::uint16_t command_id; };",
        f"inline constexpr std::size_t kGameEventActionSourceCount = {len(game_event_action_rows)};",
        "inline constexpr GameEventActionSource kGameEventActionSource[] = {",
    ]
    for uid, obj, command in game_event_action_rows:
        game_event_lines.append(f"    {{{uid}ULL, {obj}, {command}}},")
    game_event_lines += ["};", "} // namespace minidayz::stage1", ""]
    game_event_output = OUTPUT.with_name("game_events_callsite_source.hpp")
    game_event_output.parent.mkdir(parents=True, exist_ok=True)
    game_event_output.write_text("\n".join(game_event_lines), encoding="utf-8")
    print(f"Extracted {len(game_event_rows)} Game_events events and {len(game_event_action_rows)} actions.")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Extracted {len(calls)} original {FUNCTION_NAME} callsites.")
    print(f"Function definition UID: {definitions[0][1]}")
    condition_inventory = Counter((obj, command) for obj, command, *_ in function_condition_rows)
    action_inventory = Counter((obj, command) for obj, command, *_ in function_action_rows)
    print("Spawn_NPC condition ACE inventory: " + ", ".join(
        f"{obj}:{command}x{count}" for (obj, command), count in sorted(condition_inventory.items())))
    print("Spawn_NPC action ACE inventory: " + ", ".join(
        f"{obj}:{command}x{count}" for (obj, command), count in sorted(action_inventory.items())))

if __name__ == "__main__":
    main()
