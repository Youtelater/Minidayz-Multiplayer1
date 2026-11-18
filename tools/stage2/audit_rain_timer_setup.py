#!/usr/bin/env python3
"""Locate exact Construct actions which arm/reset the Rain timer."""
from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[2]
text=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
data=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",text))
def walk(events,anc=()):
    for e in events if isinstance(events,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,anc
        if len(e)>7:yield from walk(e[7],(*anc,e))
count=0
for sheet in data["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,anc in walk(sheet[1]):
        for a in e[6] if isinstance(e[6],list) else ():
            if not isinstance(a,list) or len(a)<6 or a[0]!=270:continue
            if '"Rain"' not in json.dumps(a[5],separators=(",",":")):continue
            count+=1
            print("SOURCE_RAIN_TIMER_ACTION",json.dumps({
                "sheet":sheet[0],"uid":e[4],
                "parent_uids":[v[4] for v in anc],
                "guards":[v[5] for v in anc[-3:]]+[e[5]],
                "action":a
            },separators=(",",":"),ensure_ascii=False)[:3500])
print("SOURCE_RAIN_TIMER_ACTION_COUNT",count)
