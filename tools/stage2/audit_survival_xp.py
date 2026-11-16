#!/usr/bin/env python3
"""Trace source-survival XP earn events and their complete guard ancestry."""
from pathlib import Path
import json, re

root=Path(__file__).resolve().parents[2]
source=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",source))
targets={662365640635533,609771835564225,673189388304106,571800664853456,
         184273689853649,853523169384096,422420671955084}

def walk(rows,parents=()):
    for e in rows if isinstance(rows,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        if len(e)>7:yield from walk(e[7],(*parents,e))

found={}
for sheet in project["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,parents in walk(sheet[1]):
        if e[4] in targets:
            found.setdefault(e[4],[]).append({"event_uid":e[4],
                "sheet":sheet[0],"condition":e[5],"actions":e[6],
                "parents":[{"uid":p[4],"conditions":p[5],"actions":p[6]}
                           for p in parents]})
if set(found)!=targets:
    raise SystemExit(f"Original survival XP events missing {targets-set(found)}")
for uid in sorted(found):
    for obj in found[uid][:4]:
        print("EXPERIENCE EVENT",uid, "SOURCE",
              json.dumps(obj,separators=(",",":"))[:6500])
out=root/"data/stage2/original_survival_xp_awards.json"
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(found,separators=(",",":")),encoding="utf-8")
print("Original source survival/item XP records",len(found))
