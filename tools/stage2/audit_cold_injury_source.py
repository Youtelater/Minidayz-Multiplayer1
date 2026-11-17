#!/usr/bin/env python3
"""Audit original cold injury thresholds and guarded player-health updates."""
from pathlib import Path
import json
import re

root=Path(__file__).resolve().parents[2]
raw=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
proj=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))
def walk(rows,parents=()):
    for e in rows if isinstance(rows,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(e[7] if len(e)>7 else [],(*parents,e))
count=0
for sheet in proj["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,parents in walk(sheet[1]):
        guards=e[5] if isinstance(e[5],list) else []
        touched=any(
            isinstance(c,list) and len(c)>9 and (
                '"Player_temperature"' in json.dumps(c[9]) or
                c[0] == 181 and any(
                    isinstance(x,list) and x==[10,16] for x in c[9]))
            for c in guards)
        if not touched:continue
        action=e[6] if isinstance(e[6],list) else []
        relevant=[a for a in action if
                  isinstance(a,list) and len(a)>5 and (
                      (a[0]==181 and '[10,15]' in json.dumps(a[5])) or
                      'Player_temperature' in json.dumps(a[5]))]
        if count<65:
            print("COLD_GUARD",json.dumps({
                "sheet":sheet[0],"uid":e[4],
                "conditions":guards,
                "ancestors":[{"uid":p[4],"conditions":p[5]} for p in parents[-4:]],
                "actions":relevant if relevant else action[:5],
            },separators=(",",":"))[:3600])
        count+=1
print("COLD_GUARD_TOTAL",count)
