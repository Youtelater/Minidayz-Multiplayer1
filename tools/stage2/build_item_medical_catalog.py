#!/usr/bin/env python3
"""Translate only three verified original Construct medical item actions.

Root original sources only: data.deobfuscated.js parsed project and root XML.
18/26 clear Player instance variable 22 when it is 1 (bleeding).
46 clears Player instance variable 35 (infection).
Their exact Wpn_cooldown calls are also retained as source metadata.
"""
from __future__ import annotations
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path
from build_item_effect_catalog import GROUPS, children, find_uid, item_parameter

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
ITEMS = ROOT / "l_eng_items.xml"
OUTPUT = ROOT / "include/minidayz/generated/item_medical_catalog.hpp"

EXPECTED = {
    18: ("Bandage", 22, "stop_bleeding", 2),
    26: ("Rags", 22, "stop_bleeding", 5),
    46: ("Tetracycline", 35, "stop_infection", 3),
}

def walk(events):
    for event in events if isinstance(events, list) else ():
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        yield event
        if len(event) > 7:
            yield from walk(event[7])

def is_write_zero(action, variable):
    return (isinstance(action, list) and len(action) >= 6 and
            action[0:2] == [181, 88] and
            [10, variable] in action[5] and
            [7, [0, 0]] in action[5])

def has_bleeding_condition(event):
    return any(isinstance(cond, list) and len(cond) >= 10 and
               cond[0:2] == [181, 71] and
               cond[9] == [[10, 22], [8, 0], [7, [0, 1]]]
               for cond in event[5])

def cooldown(action):
    if not isinstance(action, list) or len(action) < 6 or action[0:2] != [189, 69]:
        return None
    params = action[5]
    if (not isinstance(params, list) or len(params) < 2 or
        params[0] != [1, [2, "Wpn_cooldown"]]):
        return None
    args = params[1]
    if not isinstance(args, list) or len(args) != 2 or args[0] != 13:
        return None
    value = args[1]
    if (isinstance(value, list) and len(value) == 2 and value[0] == 7 and
        isinstance(value[1], list) and len(value[1]) == 2 and value[1][0] == 0):
        return value[1][1]
    return None

def main():
    names = {int(row.findtext("id")): (row.findtext("name") or "").strip()
             for row in ET.parse(ITEMS).getroot().findall("item")}
    source = PROJECT.read_text(encoding="utf-8-sig")
    source = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", source)
    root = json.loads(source)
    game = next((sheet for sheet in root["project"][6]
                 if isinstance(sheet, list) and len(sheet) > 1 and
                 sheet[0] == "Game_events"), None)
    if not game:
        raise SystemExit("Original Game_events missing")
    group = find_uid(game[1], 8381242860269222)  # original item_activate
    if group is None:
        raise SystemExit("Original item_activate missing")
    rows = []
    for event in children(group):
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        item_id = item_parameter(event)
        if item_id not in EXPECTED:
            continue
        label, variable, behavior, seconds = EXPECTED[item_id]
        if names.get(item_id) != label:
            raise SystemExit(f"Original source item changed: {item_id}")
        matches = []
        for node in walk([event]):
            actions = node[6] if isinstance(node[6], list) else []
            if not any(is_write_zero(a, variable) for a in actions):
                continue
            if behavior == "stop_bleeding" and not has_bleeding_condition(node):
                raise SystemExit(f"Bandage/rags source bleeding condition changed: {item_id}")
            use_cooldowns = [v for a in actions if (v := cooldown(a)) is not None]
            if use_cooldowns != [seconds]:
                raise SystemExit(f"Source medical cooldown changed: {item_id}: {use_cooldowns}")
            if not any(isinstance(a, list) and len(a) >= 2 and
                       a[0:2] == [189, 272] for a in actions):
                raise SystemExit(f"Original source does not consume medical item {item_id}")
            matches.append((node[4], seconds))
        if len(matches) != 1:
            raise SystemExit(f"Expected exactly one original medical event {item_id}, found {matches}")
        rows.append((item_id, behavior, matches[0][0], seconds))
    rows.sort()
    if len(rows) != len(EXPECTED):
        raise SystemExit(f"Missing medical item IDs: {set(EXPECTED) - {r[0] for r in rows}}")
    lines = [
        "#pragma once", "#include <cstdint>", "#include <cstddef>",
        "namespace minidayz::items {",
        "enum class SourceMedicalAction : std::uint8_t { stop_bleeding, stop_infection };",
        "struct SourceMedicalRule { std::uint16_t item_id; SourceMedicalAction action; std::uint8_t cooldown_seconds; std::uint64_t event_uid; };",
        f"inline constexpr std::size_t kSourceMedicalRuleCount = {len(rows)};",
        "inline constexpr SourceMedicalRule kSourceMedicalRules[] = {",
    ]
    for item_id, behavior, uid, seconds in rows:
        lines.append(f"    {{{item_id}, SourceMedicalAction::{behavior}, {seconds}, {uid}ULL}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Extracted and validated {len(rows)} original medical actions: {rows}")

if __name__ == "__main__":
    main()
