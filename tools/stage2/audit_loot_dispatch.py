#!/usr/bin/env python3
"""Audit the precise original Trigger_spawn -> Spawn_drop category dispatch.

Capture all conditional ancestry. Do not translate unhandled comparisons to
invented defaults, and never assign a loot category by filename/appearance.
"""
import json
import re
from collections import Counter
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SRC=ROOT/"data/stage1/construct_project_parsed.json"
OUT=ROOT/"data/stage2/original_loot_dispatch.json"
ROOT_UID=3772264929481614

def child(e):
    return e[7] if isinstance(e,list) and len(e)>7 and isinstance(e[7],list) else []
def walk(events,parents=()):
    for event in events if isinstance(events,list) else ():
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:continue
        yield event,parents
        yield from walk(child(event),(*parents,event))
def literal(expr):
    if isinstance(expr,list) and len(expr)==2 and expr[0]==0:
        return expr[1]
    return None
def marker_guard(guard):
    if not (isinstance(guard,list) and len(guard)>9 and
            guard[0:2]==[218,71]):return None
    args=guard[9]
    if len(args)<3 or args[0]!=[10,0]:return None
    return {"operator":args[1],"literal":literal(args[2][1]) if
            isinstance(args[2],list) and len(args[2])==2 else None}
def globals_guard(guard):
    if not(isinstance(guard,list) and len(guard)>9 and
           guard[0:2]==[-1,46]):return None
    args=guard[9]
    if len(args)<3 or not isinstance(args[0],list) or args[0][0]!=11:return None
    return {"name":args[0][1],"operator":args[1],
            "value":literal(args[2][1]) if isinstance(args[2],list) and
            len(args[2])==2 else None}
def main():
    text=SRC.read_text(encoding="utf-8-sig")
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",text))
    roots=[e for sheet in project["project"][6] if isinstance(sheet,list) and len(sheet)>1
           for e,_ in walk(sheet[1]) if e[4]==ROOT_UID]
    if len(roots)!=1:raise SystemExit("Original LOOT Respawn tree missing")
    output=[]
    for e,parents in walk([roots[0]]):
        for a in e[6] if isinstance(e[6],list) else ():
            if (not isinstance(a,list) or len(a)<6 or a[0:2]!=[189,69] or
                not isinstance(a[5],list) or len(a[5])<2 or
                a[5][0]!=[1,[2,"Spawn_drop"]]):continue
            ancestors=[p[5] for p in parents]+[e[5]]
            conds=[c for group in ancestors for c in group]
            category=[g for c in conds if (g:=marker_guard(c)) is not None]
            flags=[g for c in conds if (g:=globals_guard(c)) is not None]
            other=[c for c in conds if marker_guard(c) is None and
                   globals_guard(c) is None]
            row={"event":e[4],"action":a[3],
                 "category":category,"flags":flags,"other":other,
                 "parents":[p[4] for p in parents],"params":a[5][1]}
            output.append(row)
    if len(output)<50:raise SystemExit("Original loot action coverage shrank")
    counter=Counter()
    for x in output:
        marker=tuple((v["operator"][1] if isinstance(v["operator"],list) else 9,
                      v["literal"]) for v in x["category"])
        pubg=tuple((g["operator"][1] if isinstance(g["operator"],list) else 9,
                    g["value"]) for g in x["flags"] if g["name"]=="SEED_pubg")
        counter[(str(marker),str(pubg),len(x["other"]))]+=1
        if len(x["other"])<=2:
            print("LOOT SIMPLE",x["event"],"marker",marker,"SEED_pubg",pubg,
                  "other",str(x["other"])[:700],"action",x["action"])
    print("Original loot dispatch patterns",len(counter))
    for group,count in counter.most_common():
        print("DISPATCH",count,group)
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(output,separators=(",",":"),ensure_ascii=False),
                   encoding="utf-8")
    print("Exact original Spawn_drop ancestry rows",len(output))
if __name__=="__main__":main()
