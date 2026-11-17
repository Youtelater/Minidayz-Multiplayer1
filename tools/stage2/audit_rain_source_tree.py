#!/usr/bin/env python3
"""Inspect exact MiniDAYZ Construct Rain timer and its event guards/actions.

Never derive a periodic native weather interval from arbitrary observations.
"""
import json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",s))
TARGET=7113031514534401
def walk(arr,anc=()):
    for e in arr if isinstance(arr,list) else ():
        if isinstance(e,list) and len(e)>=7 and e[0]==0:
            yield e,anc
            if len(e)>7:yield from walk(e[7],(*anc,e))
found=[]
for sh in project["project"][6]:
    if isinstance(sh,list) and len(sh)>1:
        for e,anc in walk(sh[1]):
            if e[4]==TARGET:found.append((e,anc))
if len(found)!=1:raise SystemExit("Expected exact source Rain group")
root,ancestry=found[0]
for e,anc in walk([root]):
    print("SOURCE_RAIN_EVENT",json.dumps({"uid":e[4],"parents":[v[4] for v in anc],
          "guards":e[5],"actions":e[6]},separators=(",",":"),ensure_ascii=False)[:6800])
print("SOURCE_RAIN_EVENT_TOTAL",sum(1 for _ in walk([root])))
