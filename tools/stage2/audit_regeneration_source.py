#!/usr/bin/env python3
"""Trace exact original regeneration and cold-damage actions with all event guards."""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
raw=(ROOT/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\\s+(?=\\d)","-",raw))
terms=("Regeneration_current","perk_regen","Player_temperature","Night_cold","Rain_cold","Stats_blood_lost")
def walk(events,parents=()):
    for event in events if isinstance(events,list) else ():
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:continue
        yield event,parents
        yield from walk(event[7] if len(event)>7 else [],(*parents,event))
for sheet in project["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for event,parents in walk(sheet[1]):
        hits=[]
        for action in event[6] if isinstance(event[6],list) else ():
            text=json.dumps(action,separators=(",",":"))
            if any(k in text for k in terms): hits.append(action)
        if not hits:continue
        for action in hits:
            p=action[5] if len(action)>5 else []
            if not any(k in json.dumps(p) for k in ("Regeneration_current","perk_regen","Night_cold","Rain_cold")):continue
            result={"sheet":sheet[0],"uid":event[4],"conditions":event[5],"ancestor_conditions":[{"uid":p[4],"conditions":p[5]} for p in parents[-6:]],"action":action}
            print("SOURCE_REGEN_OR_WEATHER",json.dumps(result,separators=(",",":"))[:6000])
