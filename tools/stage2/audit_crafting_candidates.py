#!/usr/bin/env python3
"""Show the precise original Construct event branches for outstanding crafts.

No recipes are inferred here: each condition/action retains its source encoding
and event UID. This report is for converting selected functions without guessing.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "data/stage2/crafting_candidate_trace.json"
WANTED = {
    "Craft_ak", "Craft_rail", "Craft_tent", "Craft_xiaoyinqi",
    "Craft_fishing_rod", "Craft_molotov_flare", "Craft_molotov_whiskey",
    "Craft_31", "Craft_32", "Craft_33", "Craft_34",
    "Craft_bed", "Craft_bed1"
}

def walk(events):
    for event in events if isinstance(events, list) else []:
        if isinstance(event,list) and len(event)>6 and event[0]==0:
            yield event
            if len(event)>7:
                yield from walk(event[7])

def function_name(event):
    for cond in event[5]:
        if isinstance(cond,list) and len(cond)>9 and cond[:2]==[292,71]:
            if [10,0] in cond[9]:
                for arg in cond[9]:
                    if isinstance(arg,list) and arg[:1]==[7] and isinstance(arg[1],list) and arg[1][0]==2:
                        return arg[1][1]
    return None

def main():
    data=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
                           SRC.read_text(encoding="utf-8-sig")))
    found={}
    for sheet in data["project"][6]:
        if not isinstance(sheet,list) or len(sheet)<2:continue
        for event in walk(sheet[1]):
            name=function_name(event)
            if name in WANTED:
                if name in found:raise SystemExit(f"Repeated function {name}")
                found[name]=event
    if set(found)!=WANTED:raise SystemExit(f"Missing crafts: {WANTED-set(found)}")
    output=[]
    for name in sorted(WANTED):
        rows=[]
        for e in walk([found[name]]):
            row={"uid":e[4],"conds":e[5],"actions":e[6]}
            rows.append(row)
        output.append({"name":name,"uid":found[name][4],"events":rows})
        print("CRAFT SOURCE",name,"uid",found[name][4],"events",len(rows),flush=True)
        for idx,row in enumerate(rows):
            conditions=[(c[:2],c[9]) for c in row["conds"] if isinstance(c,list) and len(c)>9]
            actions=[(a[:2],a[5] if len(a)>5 else []) for a in row["actions"] if isinstance(a,list)]
            # Keep all literal source predicates, including unique inventory
            # item IDs/quantities and placed-object tests for accuracy.
            print(f"  {idx:02d} uid={row['uid']} conditions={json.dumps(conditions,separators=(',',':'))[:1250]} actions={json.dumps(actions,separators=(',',':'))[:900]}",flush=True)
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(output,separators=(",",":"),ensure_ascii=False),encoding="utf-8")

if __name__=="__main__":main()
