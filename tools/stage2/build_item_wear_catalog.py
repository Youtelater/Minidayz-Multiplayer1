#!/usr/bin/env python3
"""Generate exact per-action condition wear from original Construct events.

Only supported original ACE expression forms are translated. Each row carries
the source action UID, not just its often-duplicated enclosing event UID.
Its surrounding Construct event conditions must be evaluated by the caller;
this catalog must not be used to guess attack or hit timing.
"""
from __future__ import annotations

import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/item_wear_catalog.hpp"

FIXED, PERK_FIXED, RANDOM, PERK_RANDOM, CRAFTED = range(5)


def literal(expr: object) -> float | None:
    if isinstance(expr, list) and len(expr) == 2 and expr[0] in (0, 1):
        if type(expr[1]) in (int, float):
            value = float(expr[1])
            return value if math.isfinite(value) else None
    return None


def floor_random(expr: object) -> tuple[float, float] | None:
    # Construct floor(random(min, max)); original function opcodes 80/95.
    if (not isinstance(expr, list) or len(expr) != 3 or expr[:2] != [19, 80]):
        return None
    nested = expr[2]
    if not isinstance(nested, list) or len(nested) != 1:
        return None
    call = nested[0]
    if not isinstance(call, list) or len(call) != 3 or call[:2] != [19, 95]:
        return None
    args = call[2]
    if not isinstance(args, list) or len(args) != 2:
        return None
    low, high = literal(args[0]), literal(args[1])
    return (low, high) if low is not None and high is not None and high > low else None


def rule(expr: object) -> tuple[int, float, float] | None:
    value = literal(expr)
    if value is not None and value >= 0:
        return FIXED, value, 0.0
    bounds = floor_random(expr)
    if bounds:
        return RANDOM, *bounds
    if (isinstance(expr, list) and len(expr) == 3 and
            expr[0] == 6 and literal(expr[1]) is not None and
            expr[2] == [23, "Crafted"]):
        return CRAFTED, literal(expr[1]), 0.0

    if not isinstance(expr, list) or len(expr) != 4 or expr[0] != 18:
        return None
    condition, yes, no = expr[1:]
    if not isinstance(condition, list) or len(condition) != 3 or condition[0] != 12:
        return None
    if condition[1] != [23, "perk_wxnj"]:
        return None
    selected = literal(condition[2])
    if selected == 1.0 and literal(yes) == 0.0:
        value = literal(no)
        if value is not None and value >= 0:
            return PERK_FIXED, value, 0.0
    if selected == 0.0 and literal(no) == 0.0:
        bounds = floor_random(yes)
        if bounds:
            return PERK_RANDOM, *bounds
    return None


def events(node: object):
    if not isinstance(node, list):
        return
    if len(node) >= 7 and node[0] == 0 and isinstance(node[6], list):
        yield node
        if len(node) > 7:
            for child in node[7]:
                yield from events(child)
        return
    for child in node:
        if isinstance(child, list):
            yield from events(child)


def main() -> None:
    text = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
                  PROJECT.read_text(encoding="utf-8-sig"))
    project = json.loads(text)["project"]
    sheet = next((s for s in project[6] if s[0] == "Game_events"), None)
    if sheet is None:
        raise SystemExit("Missing original Game_events")
    rows: dict[int, tuple[int, float, float]] = {}
    actions = 0
    unsupported = []
    for event in events(sheet[1]):
        for action in event[6]:
            if not (isinstance(action, list) and len(action) > 5 and
                    action[0] == 1030 and action[1] == 87):
                continue
            params = action[5]
            if not (isinstance(params, list) and len(params) >= 2 and
                    params[0] == [10, 6] and
                    isinstance(params[1], list) and len(params[1]) == 2 and
                    params[1][0] == 7):
                continue
            actions += 1
            action_uid = action[3]
            decoded = rule(params[1][1])
            if decoded is None:
                unsupported.append(action_uid)
                continue
            if action_uid in rows and rows[action_uid] != decoded:
                raise SystemExit(f"Conflicting source wear rules for {action_uid}")
            rows[action_uid] = decoded

    if actions < 140 or len(rows) < 100:
        raise SystemExit(f"Unexpected source wear coverage: {actions} actions, {len(rows)} rules")
    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct SourceWearRule { std::uint64_t action_uid; std::uint8_t kind; float low; float high; };",
        f"inline constexpr std::size_t kSourceWearRuleCount = {len(rows)};",
        "inline constexpr SourceWearRule kSourceWearRules[] = {",
    ]
    def cpp_float(value: float) -> str:
        formatted = format(value, ".8g")
        if "." not in formatted and "e" not in formatted:
            formatted += ".0"
        return formatted + "f"

    for uid, (kind, low, high) in sorted(rows.items()):
        lines.append(f"    {{{uid}ULL, {kind}, {cpp_float(low)}, {cpp_float(high)}}},")
    lines.extend(["};", "} // namespace minidayz::items", ""])
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Source condition actions: {actions}; decoded unique actions: {len(rows)}; unsupported: {len(unsupported)}")
    if unsupported:
        print("Unsupported action UIDs:", ", ".join(map(str, sorted(set(unsupported)))))


if __name__ == "__main__":
    main()
