#!/usr/bin/env python3
"""Extract the original backpack grid size from the Construct Items_update events."""
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/backpack_capacity_catalog.hpp"
EXPECTED_SLOTS = list(range(1, 12))

def children(event):
    return event[7] if isinstance(event, list) and len(event) > 7 and isinstance(event[7], list) else []

def events(entries):
    for event in entries if isinstance(entries, list) else []:
        if not isinstance(event, list) or not event or event[0] != 0:
            continue
        yield event
        yield from events(children(event))

def function_name(event):
    conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
    for condition in conditions:
        if not isinstance(condition, list) or len(condition) < 10:
            continue
        if condition[0] == 189 and condition[1] == 40:
            params = condition[9]
            if isinstance(params, list) and params and isinstance(params[0], list):
                value = params[0]
                if len(value) > 1 and isinstance(value[1], list) and len(value[1]) > 1:
                    return value[1][1]
    return None

def main() -> None:
    if not PROJECT.is_file():
        raise SystemExit(f"Missing original Construct project: {PROJECT}")
    source = PROJECT.read_text(encoding="utf-8-sig")
    source = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", source)
    project = json.loads(source)
    sheets = project["project"][6]
    matches = [event for sheet in sheets for event in events(sheet[1])
               if function_name(event) == "Items_update"]
    if len(matches) != 1:
        raise SystemExit(f"Expected one original Items_update function, found {len(matches)}")
    item_update = matches[0]
    backpack_indices = []
    for event in events(children(item_update)):
        actions = event[6] if len(event) > 6 and isinstance(event[6], list) else []
        for action in actions:
            if not isinstance(action, list) or len(action) < 6 or action[0] != 189 or action[1] != 69:
                continue
            params = action[5]
            if not isinstance(params, list) or len(params) < 2:
                continue
            call = params[0]
            if not (isinstance(call, list) and len(call) > 1 and
                    isinstance(call[1], list) and len(call[1]) > 1 and call[1][1] == "Drop_slot"):
                continue
            args = params[1]
            if not isinstance(args, list) or len(args) < 3:
                continue
            index_expr, category_expr = args[1], args[2]
            if not (isinstance(index_expr, list) and len(index_expr) > 1 and
                    isinstance(index_expr[1], list) and len(index_expr[1]) == 2 and
                    index_expr[1][0] == 0 and isinstance(index_expr[1][1], int)):
                continue
            if not (isinstance(category_expr, list) and len(category_expr) > 1 and
                    isinstance(category_expr[1], list) and len(category_expr[1]) == 2 and
                    category_expr[1][0] == 2 and category_expr[1][1] == "Backpack"):
                continue
            backpack_indices.append(index_expr[1][1])
    actual = sorted(backpack_indices)
    if actual != EXPECTED_SLOTS:
        raise SystemExit(f"Original backpack grid changed: expected {EXPECTED_SLOTS}, found {actual}")
    lines = [
        "#pragma once", "#include <cstddef>",
        "namespace minidayz::items {",
        f"inline constexpr std::size_t kSourceBackpackStorageSlotCount = {len(actual)};",
        "} // namespace minidayz::items", ""
    ]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\\n".join(lines), encoding="utf-8")
    print(f"Extracted {len(actual)} original backpack storage slots from Items_update")

if __name__ == "__main__":
    main()
