#!/usr/bin/env python3
"""Source-audit original non-menu perk mechanics and exact game event guards."""
from pathlib import Path
from collections import defaultdict
import json
import re

root=Path(__file__).resolve().parents[2]
raw=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
proj=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))
perks=("header","spotter","runner","camel","hamster","snowborn",
       "farmer","regen","hunter","meleeatk","meleedef","trunkster",
       "sweethome","metabolism","eye","agronom")
def walk(rows,parents=()):
    for e in rows if isinstance(rows,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(e[7] if len(e)>7 else [],(*parents,e))
count=defaultdict(int)
sample=defaultdict(list)
for sheet in proj["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,parents in walk(sheet[1]):
        if any(p[4]==680413204391849 for p in parents):continue
        for perk in perks:
            term='"perk_'+perk+'"'
            if term not in json.dumps([e[5],e[6]],separators=(",",":")):continue
            count[perk]+=1
            if len(sample[perk])<8:
                sample[perk].append({"sheet":sheet[0],"uid":e[4],
                                     "parents":[p[4] for p in parents[-4:]],
                                     "guards":e[5],"actions":e[6]})
for perk in perks:
    print("SOURCE_PERK_MECHANIC",perk,"count",count[perk])
    for sample_row in sample[perk]:
        print("PERK_DETAIL",perk,json.dumps(sample_row,separators=(",",":"))[:2600])
