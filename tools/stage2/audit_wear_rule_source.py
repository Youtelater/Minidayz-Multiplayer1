#!/usr/bin/env python3
"""Inspect exact original item durability-wear expressions in six event groups."""
import json
from pathlib import Path
from collections import Counter
from validate_item_condition_trace import GROUPS, is_condition_write

ROOT = Path(__file__).resolve().parents[2]
TRACE = ROOT / "data/stage2/item_behavior_trace.json"
OUT = ROOT / "data/stage2/condition_wear_source_audit.json"

def main():
    trace = json.loads(TRACE.read_text(encoding="utf-8"))
    groups = {v["source_uid"]:v for v in trace["groups"]}
    report = []
    for uid,(label, required) in GROUPS.items():
        seen = {}
        for event in groups[uid]["events"]:
            for action in event.get("actions",[]):
                if not is_condition_write(action):continue
                expression = action[5]
                key = json.dumps(expression, separators=(",", ":"))
                seen.setdefault(key,[]).append((event["uid"],action[3]))
                report.append({"group_uid":uid,"label":label,
                    "event_uid":event["uid"],"parent_uid":event.get("parent_uid"),
                    "conditions":event["conditions"],"action":action})
        print(f"ORIGINAL WEAR {label}: {len(seen)} distinct expressions / "
              f"{sum(len(v) for v in seen.values())} source writes")
        for expr, sources in sorted(seen.items(), key=lambda row:-len(row[1]))[:24]:
            print(f"  SOURCE WEAR ACTION {len(sources)}x "
                  f"event_uid={sources[0][0]} action_uid={sources[0][1]} "
                  f"original_expr={expr[:2800]}")
        for event in report:
            if event["group_uid"] == uid and event["event_uid"] == required:
                print("  REPRESENTATIVE ORIGINAL CONDITIONAL ACTION:",
                      json.dumps(event, separators=(",", ":"))[:3000])
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(report, separators=(",", ":")),encoding="utf-8")
    print(f"Original source wear write actions: {len(report)}")

if __name__=="__main__":main()
