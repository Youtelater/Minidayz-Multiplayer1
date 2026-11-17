#!/usr/bin/env python3
"""Source-localized satchel, supplies stash item and category event audit."""
import json
import re
from pathlib import Path

root=Path(__file__).resolve().parents[2]
raw=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))
def walk(events,parents=()):
    for e in events if isinstance(events,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(e[7] if len(e)>7 else [],(*parents,e))
for item in (458,459,460,461,752,753,754):
    count=0
    for sheet in project["project"][6]:
        if not isinstance(sheet,list) or len(sheet)<2:continue
        for e,parents in walk(sheet[1]):
            payload=json.dumps([e[5],e[6]],separators=(",",":"))
            hits=(f"[0,{item}]" in payload or
                  f"[2,\"{item}\"]" in payload)
            if not hits:continue
            if count<18:
                print("SOURCE_PACK_STASH",item,json.dumps({
                    "sheet":sheet[0],"uid":e[4],"parents":[{"uid":p[4],"conditions":p[5]} for p in parents[-5:]],
                    "conditions":e[5],"actions":e[6]},separators=(",",":"))[:6000])
            count+=1
    print("SOURCE_PACK_STASH_TOTAL",item,count)
