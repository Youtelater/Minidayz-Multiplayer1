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
ARMY=(0,24,42,43,44,20,55,125)
MEDICAL=(0,18,19,28,29,45,46,109,118,708,710,143,759,760)
EXPECTED={
    315583148629362:("perk_kb2",ARMY,10),
    699699206748072:("perk_kb3",ARMY,10),
    962634450988744:("perk_kb4",ARMY,10),
    926789927221036:("perk_kb2",MEDICAL,11),
    353303031815206:("perk_kb3",MEDICAL,11),
    250048865204980:("perk_kb4",MEDICAL,11),
}
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
        source_name, whitelist, wearer_type = EXPECTED[e[4]]
        if conditions != [[source_name,[8,1],item] for item in whitelist]:
            raise SystemExit(f"Source restricted item set changed for {e[4]}: {conditions}")
        if not any(
            isinstance(c,list) and len(c)>9 and c[0:2]==[181,71] and
            c[9]==[[10,9],[8,0],[7,[0,wearer_type]]]
            for p in parents for c in p[5] if isinstance(p[5],list)):
            raise SystemExit(f"Original satchel wearer mapping changed for {e[4]}")
        print("EXCLUSIVE_CATEGORY",e[4],json.dumps(conditions,separators=(",",":")),
              "parents",json.dumps([{"uid":p[4],"guards":p[5]} for p in parents[-2:]],separators=(",",":"))[:2200])
if found!=TARGETS:raise SystemExit(f"Missing exclusive categories {TARGETS-found}")
