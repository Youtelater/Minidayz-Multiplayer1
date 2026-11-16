#!/usr/bin/env python3
"""Extract exact original fractional held-item wear actions and ancestors.

Any value not literally known stays a raw Construct expression. This file
does not manufacture a wear rule or treat nested event guards as unconditional.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "data/stage2/original_item_wear_actions.json"
GROUPS = {
    "firearm": (6904772320561696, 875054325568454),
    "alternate": (337954396197555, 875054325568454),
    "melee": (3528035038609865, 2098275646157213),
    "damage": (811368909310732, 251602875520018),
    "player_damage": (9520121190252054, 682686209618980),
    "obstacle": (5836740205504893, 8036449108462062),
}

def children(e):
    return e[7] if isinstance(e,list) and len(e)>7 and isinstance(e[7],list) else []

def walk(rows, ancestry=()):
    for e in rows if isinstance(rows,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e, ancestry
        yield from walk(children(e), (*ancestry,e))

def is_wear_write(a):
    return (isinstance(a,list) and len(a)>=6 and a[0]==1030 and
            a[1] in (87,88) and isinstance(a[5],list) and
            len(a[5])>=2 and a[5][0]==[10,6])

def main():
    text=SOURCE.read_text(encoding="utf-8-sig")
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",text))
    events={}
    for sheet in project["project"][6]:
        if not isinstance(sheet,list) or len(sheet)<2:continue
        for event,ancestry in walk(sheet[1]):
            events.setdefault(event[4],[]).append((event,ancestry))
    output=[]
    for name,(group_uid,required_uid) in GROUPS.items():
        roots=events.get(group_uid,[])
        if len(roots)!=1:
            raise SystemExit(f"Original {name} group changed; matches={len(roots)}")
        records=[]
        for event,ancestry in walk([roots[0][0]]):
            writes=[a for a in event[6] if is_wear_write(a)]
            if not writes:continue
            guard=[{"uid":parent[4],"conditions":parent[5]} for parent in ancestry]
            detail={"name":name,"uid":event[4],"conditions":event[5],
                    "ancestors":guard,"writes":writes}
            records.append(detail)
            summary=[{"method":a[1],"expression":a[5][1]}
                     for a in writes]
            print(f"WEAR {name} event={event[4]} guard={json.dumps(guard[-3:],separators=(',',':'))[:1600]}")
            print(f"  actions: {json.dumps(summary,separators=(',',':'))[:2400]}")
        if not any(x["uid"]==required_uid for x in records):
            raise SystemExit(f"Required original {name} source UID missing: {required_uid}")
        print(f"WEAR GROUP {name}: {len(records)} guarded wear events")
        output+=records
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text(json.dumps(output,separators=(",",":"),ensure_ascii=False),encoding="utf-8")
    print(f"Wear events captured: {len(output)}")

if __name__=="__main__": main()
