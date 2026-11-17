#!/usr/bin/env python3
"""Trace original satchel-exclusive inventory predicates and item filters."""
import json
import re
from pathlib import Path

root=Path(__file__).resolve().parents[2]
raw=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
proj=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))
def walk(rows,parents=()):
    for e in rows if isinstance(rows,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(e[7] if len(e)>7 else [],(*parents,e))
n=0
for sheet in proj["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,parents in walk(sheet[1]):
        data=json.dumps([e[5],e[6]],separators=(",",":"))
        if not any(term in data for term in ("perk_kb1","perk_kb2","perk_kb","player_backpacks")):
            continue
        if n<110:
            print("SATCHEL_FILTER",json.dumps({
                "sheet":sheet[0],"uid":e[4],
                "guards":e[5],"actions":e[6],
                "ancestors":[{"uid":p[4],"guards":p[5]} for p in parents[-4:]]
            },separators=(",",":"))[:4600])
        n+=1
print("SATCHEL_FILTER_TOTAL",n)
