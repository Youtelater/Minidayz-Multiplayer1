#!/usr/bin/env python3
"""Source-trace Rain_speed initial value and all write operations."""
from pathlib import Path
import json,re
src=(Path(__file__).resolve().parents[2]/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
data=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",src))
def walk(arr,parents=()):
    for e in arr if isinstance(arr,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        if len(e)>7:yield from walk(e[7],(*parents,e))
for sheet in data["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,anc in walk(sheet[1]):
        writes=[a for a in e[6] if isinstance(a,list) and len(a)>5 and
                '"Rain_speed"' in json.dumps(a[5],separators=(",",":"))]
        if writes:
            print("RAIN_SPEED_ACTION",json.dumps({
                "sheet":sheet[0],"uid":e[4],"ancestors":[p[4] for p in anc],
                "guards":e[5],"writes":writes
            },separators=(",",":"),ensure_ascii=False)[:3100])
