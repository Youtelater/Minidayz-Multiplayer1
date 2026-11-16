#!/usr/bin/env python3
"""Find exact source perk allocation and XP conditions in original Construct.

This is a provenance extraction, not a guessed perk progression model.
"""
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"data/stage1/construct_project_parsed.json"
OUT=ROOT/"data/stage2/original_perk_actions.json"
UID=680413204391849
TARGETS=("perk_hamster","perk_camel","perk_cybb","perk_regen","perk_wxnj")
def children(e):
    return e[7] if isinstance(e,list) and len(e)>7 and isinstance(e[7],list) else []
def walk(rows,parent=()):
    for e in rows if isinstance(rows,list) else []:
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parent
        yield from walk(children(e),(*parent,e))
def main():
    text=SOURCE.read_text(encoding="utf-8-sig")
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",text))
    groups=[e for sheet in project["project"][6] if isinstance(sheet,list) and len(sheet)>1
            for e,_ in walk(sheet[1]) if e[4]==UID]
    if len(groups)!=1:raise SystemExit(f"Original perks group missing: {len(groups)}")
    output=[]
    for event,parents in walk([groups[0]]):
        data=json.dumps([event[5],event[6]],separators=(",",":"))
        if not any(name in data for name in TARGETS):continue
        row={"uid":event[4],"parents":[{"uid":p[4],"conditions":p[5]} for p in parents],
             "conditions":event[5],"actions":event[6]}
        output.append(row)
        print("PERK EVENT",event[4],"PARENTS",json.dumps(row["parents"][-3:],separators=(",",":"))[:1200])
        print("  CONDITIONS",json.dumps(event[5],separators=(",",":"))[:2500])
        print("  ACTIONS",json.dumps(event[6],separators=(",",":"))[:2500])
    if not output:raise SystemExit("Original perk actions missing")
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(output,separators=(",",":"),ensure_ascii=False),encoding="utf-8")
    print(f"Original source perk action records: {len(output)}")
if __name__=="__main__":main()
