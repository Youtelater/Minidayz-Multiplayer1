#!/usr/bin/env python3
"""Read only the repository-root original Construct export.

Preserve source conditions/actions for survival, perks, loot and item wear.
No guessed ACE semantics, RNG weights or numeric gameplay rates are emitted.
"""
from __future__ import annotations
import json
import re
from pathlib import Path
from collections import Counter

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "data/stage2/remaining_source_trace.json"
GROUPS = {
    "player_stats": 7463747062628312,
    "perks": 680413204391849,
    "loot_respawn": 3772264929481614,
    "firearm_wear": 6904772320561696,
    "melee_wear": 3528035038609865,
    "hit_obstacle": 5836740205504893,
}

def walk(events, parent=None):
    for event in events if isinstance(events, list) else ():
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        yield event, parent
        if len(event) > 7:
            yield from walk(event[7], event[4])

def main():
    if not SOURCE.is_file():
        raise SystemExit("Missing original source Construct snapshot")
    source = SOURCE.read_text(encoding="utf-8-sig")
    source = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", source)
    project = json.loads(source)
    index = {}
    for sheet in project["project"][6]:
        if isinstance(sheet, list) and len(sheet) > 1:
            for event, parent in walk(sheet[1]):
                index[event[4]] = (event, parent)
    absent = set(GROUPS.values()) - set(index)
    if absent:
        raise SystemExit(f"Original source event UIDs changed or missing: {sorted(absent)}")
    result = {}
    match_terms = (
        "Day_cold", "Night_cold", "Rain_cold", "Player_temperature",
        "Stats_blood_lost", "Regeneration_current", "perk_hamster",
        "perk_camel", "perk_cybb", "perk_regen", "Spawn_drop"
    )
    for label, uid in GROUPS.items():
        root = index[uid][0]
        data = []
        ops = Counter()
        keywords = Counter()
        for event, parent in walk([root]):
            conditions = event[5] if len(event) > 5 else []
            actions = event[6] if len(event) > 6 else []
            for action in actions if isinstance(actions, list) else ():
                if isinstance(action, list) and len(action) > 1:
                    ops[(action[0], action[1])] += 1
            strings = json.dumps([conditions, actions], ensure_ascii=False,
                                 separators=(",", ":"))
            for word in match_terms:
                if word in strings:
                    keywords[word] += 1
            data.append({"uid": event[4], "parent": parent,
                         "conditions": conditions, "actions": actions})
        result[label] = {"uid": uid, "events": data}
        print(f"{label}: {len(data)} events, {sum(ops.values())} actions")
        print(f"  op_counts: {ops.most_common(16)}")
        print(f"  keywords: {dict(keywords)}")
        # Log bounded raw evidence, not guessed interpretations.
        for word in ("Day_cold", "perk_cybb", "Spawn_drop"):
            matches = [e for e in data if word in json.dumps(
                [e["conditions"], e["actions"]], separators=(",", ":"))]
            for event in matches[:3]:
                print(f"  {word} UID={event['uid']}: "
                      f"{json.dumps(event, separators=(',', ':'))[:1400]}")
    # Resolve the original branch/condition ancestry of a representative
    # weighted LOOT Respawn call before translating any gameplay trigger.
    loot = result["loot_respawn"]
    lookup = {event["uid"]: event for event in loot["events"]}
    for target in (673982235644371, 773215391218752):
        current = lookup.get(target)
        print(f"loot source conditional ancestry for event UID {target}:")
        hops = 0
        while current is not None and hops < 30:
            print(f"  ancestor UID={current['uid']} parent={current['parent']} "
                  f"conditions={json.dumps(current['conditions'], separators=(',', ':'))[:1900]}")
            current = lookup.get(current["parent"])
            hops += 1
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(result, separators=(",", ":"), ensure_ascii=False),
                   encoding="utf-8")
    print(f"Full source trace exported for {len(result)} Stage 2 systems")

if __name__ == "__main__":
    main()
