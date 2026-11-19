#!/usr/bin/env python3
"""Inventory system source-event coverage for the native 3DS adaptation.

The Construct Game_events and GUI event sheets are the original behavior.
This is a trace/coverage report, NOT proof the native C++ executes all
of their events. Avoid assigning new invented effects to those events.
"""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
PROJECT=ROOT/"data/stage1/construct_project_parsed.json"
OUT=ROOT/"data/stage2/inventory_original_event_groups.json"
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
    PROJECT.read_text(encoding="utf-8-sig")))["project"]
events=dict(project[6])["Game_events"]

ORIGINAL={
    "Interaction":970660140616416,
    "Using inventory items":3544291115846117,
    "Picking player items":115345084350351,
    "Picking inventory items":1529638233252698,
    "Droppin items":594580094599696,
    "Stash":709191320420159,
    "Sub_menu":7834464419753373,
    "Gui_drag":795480497256684,
    "Pickpile":4618141206649221,
}
found={}
def names(event):
    result=[]
    for c in event[5] if isinstance(event[5],list) else []:
        if not isinstance(c,list) or len(c)<10 or c[:2]!=[-1,38]:
            continue
        for p in c[-1]:
            if (isinstance(p,list) and len(p)>1 and
                    isinstance(p[1],list) and len(p[1])>1 and
                    isinstance(p[1][1],str)):
                result.append(p[1][1])
    return result
def action_count(events):
    total=0
    for e in events:
        if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
        total+=len(e[6]) if isinstance(e[6],list) else 0
        total+=action_count(e[7] if isinstance(e[7],list) else [])
    return total
def walk(events,path=()):
    for e in events:
        if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
        child=e[7] if isinstance(e[7],list) else []
        group_names=names(e)
        for name in group_names:
            if name in ORIGINAL:
                if e[4]!=ORIGINAL[name]:
                    raise SystemExit(f"Original {name} UID mismatch: {e[4]}")
                if name in found:
                    raise SystemExit(f"Duplicate source event group: {name}")
                found[name]={
                    "event_uid":e[4],
                    "parent_groups":list(path),
                    "child_events":len(child),
                    "nested_actions":action_count(child),
                }
        walk(child,path+tuple(group_names))
walk(events)
if set(found)!=set(ORIGINAL):
    raise SystemExit(f"Missing original inventory groups {set(ORIGINAL)-set(found)}")
if found["Using inventory items"]["nested_actions"]<600:
    raise SystemExit("Original item-use coverage unexpectedly reduced")
if found["Gui_drag"]["nested_actions"]<100:
    raise SystemExit("Original drag events unexpectedly reduced")
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(found,indent=2)+"\n",encoding="utf-8")
print("PASS: 9 exact original inventory/GUI event groups checked")
for name,v in found.items():
    print(name,v["event_uid"],v["child_events"],v["nested_actions"])
print("WARNING: This is source-event coverage, NOT native inventory parity.")
