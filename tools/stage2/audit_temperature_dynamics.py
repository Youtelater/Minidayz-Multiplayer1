#!/usr/bin/env python3
"""Trace original actor variable 16 body warmth vs global Player_temperature weather."""
import json
import re
from pathlib import Path
root=Path(__file__).resolve().parents[2]
raw=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))
def walk(rows,parents=()):
    for e in rows if isinstance(rows,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(e[7] if len(e)>7 else [],(*parents,e))
count=0
for sheet in project["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,parents in walk(sheet[1]):
        actions=[a for a in e[6] if isinstance(a,list) and len(a)>5 and (
            (a[0]==181 and a[5] and a[5][0]==[10,16]) or
            (a[0]==-1 and a[5] and a[5][0]==[11,"Player_temperature"]))]
        if not actions:continue
        count+=1
        if count<75:
            print("BODY_TEMP_ACTION",json.dumps({
                "sheet":sheet[0],"uid":e[4],"guards":e[5],
                "ancestors":[{"uid":p[4],"guards":p[5]} for p in parents[-4:]],
                "actions":actions},separators=(",",":"))[:4200])
print("BODY_TEMP_ACTION_COUNT",count)
