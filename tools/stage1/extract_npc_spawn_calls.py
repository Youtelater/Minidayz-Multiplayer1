#!/usr/bin/env python3
"""Extract every original Construct Spawn_NPC function call and its arguments."""
import json
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
    root = json.loads(SOURCE.read_text(encoding="utf-8"))
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
    if len({(row[1], row[2]) for row in calls}) != len(calls):
        raise SystemExit("Duplicate original Spawn_NPC event/action UID pair detected")

    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::stage1 {",
        "struct NpcSpawnCallSource { const char* event_sheet; std::uint64_t event_uid; std::uint64_t action_uid; std::uint8_t argument_count; const char* arguments[4]; };",
        f"inline constexpr std::uint64_t kNpcSpawnFunctionEventUid = {definitions[0][1]}ULL;",
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
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Extracted {len(calls)} original {FUNCTION_NAME} callsites.")
    print(f"Function definition UID: {definitions[0][1]}")

if __name__ == "__main__":
    main()
