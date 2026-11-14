#!/usr/bin/env python3
"""Extract direct item-use stat effects from original Construct activation events.

Only unconditional literal changes to the original player stat variables are
translated. Perk-dependent expressions and unrelated player variables remain
in the raw behavior trace until their semantics are ported.
"""
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/item_use_effect_catalog.hpp"
GROUPS = {
    8381242860269222: False,  # item_activate / Use
    3890676334551161: True,   # item_activate2 / Use2
}
STAT_FIELDS = {13: "hunger", 14: "thirst", 15: "health", 16: "temperature"}

def children(event):
    return event[7] if isinstance(event, list) and len(event) > 7 and isinstance(event[7], list) else []

def find_uid(entries, uid):
    for event in entries if isinstance(entries, list) else []:
        if isinstance(event, list) and len(event) > 7 and event[0] == 0:
            if event[4] == uid:
                return event
            found = find_uid(children(event), uid)
            if found is not None:
                return found
    return None

def item_parameter(event):
    conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
    for condition in conditions:
        if not isinstance(condition, list) or len(condition) < 10:
            continue
        if condition[0] != 189 or condition[1] != 112:
            continue
        params = condition[9]
        if len(params) < 3 or params[0] != [0, [0, 0]] or params[1] != [8, 0]:
            continue
        value = params[2]
        if isinstance(value, list) and len(value) == 2 and value[0] == 7:
            expr = value[1]
            if isinstance(expr, list) and len(expr) == 2 and expr[0] == 0:
                if isinstance(expr[1], (int, float)):
                    return int(expr[1])
    return None

def direct_effects(event):
    effects = {name: 0.0 for name in STAT_FIELDS.values()}
    actions = event[6] if len(event) > 6 and isinstance(event[6], list) else []
    for action in actions:
        if not isinstance(action, list) or len(action) < 6:
            continue
        if action[0] != 181 or action[1] != 83:
            continue
        params = action[5]
        variable = next((p[1] for p in params if isinstance(p, list) and len(p) > 1 and p[0] == 10), None)
        value = next((p[1] for p in params if isinstance(p, list) and len(p) > 1 and p[0] == 7), None)
        if variable not in STAT_FIELDS or not isinstance(value, list) or len(value) != 2 or value[0] != 0:
            continue
        if isinstance(value[1], (int, float)):
            effects[STAT_FIELDS[variable]] += float(value[1])
    return effects

def main():
    if not PROJECT.is_file():
        raise SystemExit(f"Missing original Construct project: {PROJECT}")
    project = json.loads(PROJECT.read_text(encoding="utf-8"))
    sheets = project.get("project", [None] * 7)[6]
    game = next((s for s in sheets if isinstance(s, list) and s and s[0] == "Game_events"), None)
    if not game:
        raise SystemExit("Original Game_events sheet missing")

    rows = []
    for uid, secondary in GROUPS.items():
        group = find_uid(game[1], uid)
        if group is None:
            raise SystemExit(f"Missing original item activation event UID {uid}")
        seen = set()
        for event in children(group):
            if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
                continue
            item_id = item_parameter(event)
            if item_id is None:
                continue
            effect = direct_effects(event)
            if not any(effect.values()):
                continue
            key = (item_id, secondary)
            if key in seen:
                raise SystemExit(f"Duplicate direct item-use effect: {key}")
            seen.add(key)
            rows.append((item_id, secondary, effect))

    rows.sort(key=lambda row: (row[0], row[1]))
    if len(rows) != 40:
        raise SystemExit(f"Expected 40 direct source item-use effect rows, found {len(rows)}")

    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct ItemUseEffectDefinition { std::uint16_t item_id; bool secondary; float hunger, thirst, health, temperature; };",
        f"inline constexpr std::size_t kItemUseEffectDefinitionCount = {len(rows)};",
        "inline constexpr ItemUseEffectDefinition kItemUseEffectDefinitions[] = {",
    ]
    for item_id, secondary, effect in rows:
        values = [effect["hunger"], effect["thirst"], effect["health"], effect["temperature"]]
        formatted = ", ".join(f"{v:.9g}f" for v in values)
        lines.append(f"    {{{item_id}, {str(secondary).lower()}, {formatted}}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} direct source item-use effects from original activation events")

if __name__ == "__main__":
    main()
