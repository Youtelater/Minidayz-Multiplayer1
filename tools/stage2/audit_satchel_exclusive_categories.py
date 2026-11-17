#!/usr/bin/env python3
"""Extract exact satchel exclusive slot eligibility branch predicates from Construct root."""
import json
import re
from pathlib import Path
root=Path(__file__).resolve().parents[2]
raw=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
proj=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))
TARGETS={315583148629362,699699206748072,962634450988744,
         926789927221036,353303031815206,250048865204980}
def walk(rows, parents=()):
    for e in rows if isinstance(rows,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(e[7] if len(e)>7 else [],(*parents,e))
found=set()
for sheet in proj["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,parents in walk(sheet[1]):
        if e[4] not in TARGETS:continue
        found.add(e[4])
        conditions=[]
        for c in e[5]:
            x=c[9] if isinstance(c,list) and len(c)>9 else []
            name=x[0][1] if len(x)>0 and isinstance(x[0],list) and len(x[0])>1 and x[0][0]==11 else ""
            comparator=x[1] if len(x)>1 else None
            expr=x[2] if len(x)>2 else None
            number=expr[1][1] if (isinstance(expr,list) and len(expr)==2 and
                      isinstance(expr[1],list) and len(expr[1])==2 and
                      expr[1][0]==0) else None
            conditions.append([name,comparator,number])
        print("EXCLUSIVE_CATEGORY",e[4],json.dumps(conditions,separators=(",",":")),
              "parents",json.dumps([{"uid":p[4],"guards":p[5]} for p in parents[-2:]],separators=(",",":"))[:2200])
if found!=TARGETS:raise SystemExit(f"Missing exclusive categories {TARGETS-found}")
