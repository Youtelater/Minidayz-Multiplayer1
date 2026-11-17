#!/usr/bin/env python3
"""Generate literal source loot drop selection pools from ORIGINAL Construct.

Preserves every duplicate choose() argument as its source weight. Does not
interpret when a drop happens, invent random seeds, or silently resolve
dynamic condition/expression branches.
"""
from __future__ import annotations
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
ITEMS = ROOT / "l_eng_items.xml"
HEADER = ROOT / "include/minidayz/generated/loot_source_pools.hpp"
AUDIT = ROOT / "data/stage2/loot_spawn_expressions.json"
GROUP_UID = 3772264929481614

def children(event):
    return event[7] if isinstance(event, list) and len(event) > 7 and isinstance(event[7], list) else []

def find_uid(events, uid):
    for event in events if isinstance(events, list) else []:
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        if event[4] == uid:
            return event
        found = find_uid(children(event), uid)
        if found is not None:
            return found
    return None

def walk(events, ancestry=()):
    for event in events if isinstance(events, list) else []:
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        yield event, ancestry
        yield from walk(children(event), (*ancestry, event))

def literal_comparison(condition, obj, ace, name=None):
    if not isinstance(condition, list) or len(condition) < 10 or condition[:2] != [obj, ace]:
        return None
    args = condition[9]
    if len(args) != 3 or args[1] != [8, 0]:
        return None
    if name is not None and args[0] != [11, name]:
        return None
    if name is None and args[0] != [10, 0]:
        return None
    expr = args[2]
    if not (isinstance(expr, list) and len(expr) == 2 and expr[0] == 7 and
            isinstance(expr[1], list) and len(expr[1]) == 2 and
            expr[1][0] == 0 and isinstance(expr[1][1], int)):
        return None
    return expr[1][1]

def source_dispatch_context(event, ancestry):
    """Preserve source guard subsets, fail closed on unsupported ancestry."""
    marker = -1
    pubg = -1
    unknown = 0
    for ancestor in (*ancestry, event):
        for condition in ancestor[5]:
            category = literal_comparison(condition, 218, 71)
            if category is not None:
                if marker >= 0 and marker != category:
                    raise SystemExit("Conflicting source marker ancestry")
                marker = category
                continue
            flag = literal_comparison(condition, -1, 46, "SEED_pubg")
            if flag is not None:
                if pubg >= 0 and pubg != flag:
                    raise SystemExit("Conflicting source SEED_pubg ancestry")
                pubg = flag
                continue
            unknown += 1
    if marker > 32767 or pubg > 127 or unknown > 65535:
        raise SystemExit("Source loot guard does not fit generated metadata")
    return marker, pubg, unknown

def literal_pool(expr):
    if isinstance(expr, list) and len(expr) == 2 and expr[0] == 0 and isinstance(expr[1], int):
        return [expr[1]]
    if isinstance(expr, list) and len(expr) == 3 and expr[0:2] == [19, 82]:
        params = expr[2]
        if (isinstance(params, list) and params and all(
            isinstance(p, list) and len(p) == 2 and
            p[0] == 0 and isinstance(p[1], int) for p in params)):
            return [p[1] for p in params]
    return None

def main():
    if not PROJECT.is_file() or not ITEMS.is_file():
        raise SystemExit("Missing repository-root original Construct or item XML")
    src = PROJECT.read_text(encoding="utf-8-sig")
    src = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", src)
    project = json.loads(src)
    game = next((sheet for sheet in project["project"][6]
                 if isinstance(sheet, list) and len(sheet) > 1 and
                 sheet[0] == "Game_events"), None)
    if not game:
        raise SystemExit("Original Game_events sheet absent")
    group = find_uid(game[1], GROUP_UID)
    if group is None:
        raise SystemExit("Original LOOT Respawn group changed")
    known = {int(e.findtext("id")) for e in ET.parse(ITEMS).getroot().findall("item")}
    pools = []
    dynamic = []
    audit = []
    for event, ancestry in walk([group]):
        for action in event[6] if isinstance(event[6], list) else []:
            if not isinstance(action, list) or len(action) < 6 or action[0:2] != [189, 69]:
                continue
            parameters = action[5]
            if not isinstance(parameters, list) or len(parameters) < 2:
                continue
            if parameters[0] != [1, [2, "Spawn_drop"]]:
                continue
            args = parameters[1]
            if not isinstance(args, list) or len(args) < 2 or args[0] != 13:
                raise SystemExit(f"Unexpected Spawn_drop parameters {action[3]}")
            first = args[1]
            if not isinstance(first, list) or len(first) != 2 or first[0] != 7:
                raise SystemExit(f"Unexpected Spawn_drop first parameter {action[3]}")
            source = {"action_uid": action[3], "event_uid": event[4],
                      "result_expr": first[1], "args": args}
            audit.append(source)
            selected = literal_pool(first[1])
            if selected is None:
                dynamic.append(source)
                continue
            missing = sorted(set(selected) - known)
            if missing:
                raise SystemExit(f"Original Spawn_drop references IDs not in root XML: {missing}")
            marker, pubg, other = source_dispatch_context(event, ancestry)
            pools.append((action[3], event[4], selected, marker, pubg, other))
    if not pools:
        raise SystemExit("No literal source loot pools were found")
    # The original Construct export reuses some ACE action UIDs in cloned
    # event branches. Preserve each occurrence, never coalesce by UID.
    repeated = len(pools) - len({row[0] for row in pools})
    pools.sort()
    flat = []
    rows = []
    for action_uid, event_uid, items, marker, pubg, other in pools:
        rows.append((action_uid, event_uid, len(flat), len(items),
                     marker, pubg, other))
        flat.extend(items)
    lines = ["#pragma once", "#include <cstddef>", "#include <cstdint>",
             "namespace minidayz::items {",
             "struct SourceLootPool { std::uint64_t action_uid; std::uint64_t event_uid; std::uint32_t first; std::uint16_t choices; std::int16_t marker; std::int8_t seed_pubg; std::uint16_t other_guard_count; };",
             f"inline constexpr std::size_t kSourceLootPoolCount = {len(pools)};",
             f"inline constexpr std::size_t kSourceLootChoiceCount = {len(flat)};",
             "inline constexpr std::uint16_t kSourceLootChoices[] = {"]
    for i in range(0, len(flat), 22):
        lines.append("    " + ", ".join(map(str, flat[i:i+22])) + ",")
    lines += ["};", "inline constexpr SourceLootPool kSourceLootPools[] = {"]
    for uid, event_uid, first, count, marker, pubg, other in rows:
        lines.append(f"    {{{uid}ULL, {event_uid}ULL, {first}, {count}, {marker}, {pubg}, {other}}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    HEADER.parent.mkdir(parents=True, exist_ok=True)
    HEADER.write_text("\n".join(lines), encoding="utf-8")
    AUDIT.parent.mkdir(parents=True, exist_ok=True)
    AUDIT.write_text(json.dumps({"literal_pools": len(pools),
        "dynamic_actions": dynamic, "all_actions": audit},
        separators=(",", ":"), ensure_ascii=False), encoding="utf-8")
    print(f"Original LOOT Respawn: {len(audit)} Spawn_drop calls, "
          f"{len(pools)} literal source pools, {len(dynamic)} dynamic expressions, "
          f"{len(flat)} weighted choices, {repeated} reused source action UIDs")
    print("Original exemplar event 673982235644371:",
          [list(row[2]) for row in pools if row[1] == 673982235644371])

if __name__ == "__main__":
    main()
