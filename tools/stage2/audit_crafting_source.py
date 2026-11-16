#!/usr/bin/env python3
"""Audit original Construct craft function actions without guessing recipes.

This report preserves exact condition/action expression trees and source UIDs.
It is an extraction artifact, not a declaration that all actions are ported.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "data/stage2/craft_source_audit.json"

def children(event):
    return event[7] if isinstance(event, list) and len(event) > 7 and isinstance(event[7], list) else []

def walk(events):
    for event in events if isinstance(events, list) else []:
        if isinstance(event, list) and len(event) > 6 and event[0] == 0:
            yield event
            yield from walk(children(event))

def source_function(event):
    for condition in event[5]:
        if not isinstance(condition, list) or len(condition) < 10 or condition[0:2] != [292, 71]:
            continue
        if [10, 0] not in condition[9]:
            continue
        for param in condition[9]:
            if isinstance(param, list) and len(param) == 2 and param[0] == 7:
                expr = param[1]
                if isinstance(expr, list) and len(expr) == 2 and expr[0] == 2:
                    return expr[1]
    return None

def main():
    text = PROJECT.read_text(encoding="utf-8-sig")
    text = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", text)
    project = json.loads(text)
    result = []
    for sheet in project["project"][6]:
        if not isinstance(sheet, list) or len(sheet) < 2:
            continue
        for event in walk(sheet[1]):
            name = source_function(event)
            if not isinstance(name, str) or not name.startswith("Craft_"):
                continue
            records = []
            for nested in walk([event]):
                records.append({
                    "uid": nested[4],
                    "conditions": nested[5],
                    "actions": nested[6],
                })
            result.append({"name": name, "uid": event[4],
                           "event_count": len(records), "events": records})
    if not result:
        raise SystemExit("Original Craft_ functions not found")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(result, separators=(",", ":"), ensure_ascii=False),
                   encoding="utf-8")
    print(f"Original source Craft_ functions: {len(result)}")
    for item in sorted(result, key=lambda v: v["name"]):
        output_ids = []
        for event in item["events"]:
            for action in event["actions"]:
                if not isinstance(action, list) or len(action) < 6:
                    continue
                if action[0:2] != [189, 69] or len(action[5]) < 2:
                    continue
                if action[5][0] != [1, [2, "Spawn_drop"]]:
                    continue
                args = action[5][1]
                if isinstance(args, list) and len(args) > 1 and isinstance(args[1], list):
                    output_ids.append(args[1])
        print(f"  {item['name']}: UID={item['uid']}, events={item['event_count']}, output_expr={output_ids[:12]}")
        if item["name"] not in ("Craft_1", "Craft_4", "Craft_5"):
            print("  SOURCE RAW FIRST EVENTS:",
                  json.dumps(item["events"][:3], separators=(",", ":"))[:4800])

if __name__ == "__main__":
    main()
