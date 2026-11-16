#!/usr/bin/env python3
"""Trace real root Construct medical item actions with source event UIDs."""
import json
import re
from pathlib import Path
from collections import Counter
from build_item_effect_catalog import GROUPS, children, find_uid, item_parameter

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "data/stage2/medical_item_source_trace.json"
MEDICAL_IDS = {18, 19, 26, 28, 29, 34, 35, 39, 46, 122}

def walk(events):
    for event in events if isinstance(events, list) else ():
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        yield event
        if len(event) > 7:
            yield from walk(event[7])

def main():
    source = SRC.read_text(encoding="utf-8-sig")
    source = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", source)
    project = json.loads(source)
    sheets = project["project"][6]
    sheet = next((v for v in sheets if isinstance(v, list) and len(v) > 1 and
                 v[0] == "Game_events"), None)
    if not sheet:
        raise SystemExit("Original Game_events not found")
    records = []
    for uid, secondary in GROUPS.items():
        root = find_uid(sheet[1], uid)
        if root is None:
            raise SystemExit(f"Original item activation UID {uid} not found")
        for event in children(root):
            if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
                continue
            item_id = item_parameter(event)
            if item_id not in MEDICAL_IDS:
                continue
            nested = []
            action_counts = Counter()
            for node in walk([event]):
                for action in node[6] if isinstance(node[6], list) else ():
                    if isinstance(action, list) and len(action) >= 6:
                        action_counts[tuple(action[0:2])] += 1
                nested.append({"uid": node[4], "conditions": node[5],
                               "actions": node[6]})
            result = {"item_id": item_id, "secondary": secondary,
                      "root_uid": event[4], "events": nested}
            records.append(result)
            print(f"item={item_id} secondary={secondary}: root={event[4]} "
                  f"events={len(nested)} action_ids={sorted(action_counts.items())}")
            for node in nested:
                if node["actions"]:
                    print(f"  node={node['uid']} "
                          f"{json.dumps(node, separators=(',', ':'))[:2000]}")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(records, separators=(",", ":"), ensure_ascii=False),
                   encoding="utf-8")
    print(f"Medical item activation records: {len(records)}")

if __name__ == "__main__":
    main()
