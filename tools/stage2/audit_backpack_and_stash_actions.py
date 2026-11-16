#!/usr/bin/env python3
"""Audit original repository-root backpack/tent/stash activation event branches."""
import json
import re
from pathlib import Path
from build_item_effect_catalog import GROUPS, children, find_uid, item_parameter

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "data/stage2/backpack_and_stash_actions.json"
IDS = {450,451,452,453,454,455,456,457,458,459,460,461,462,752,753,754}

def walk(events):
    for event in events if isinstance(events,list) else []:
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:
            continue
        yield event
        if len(event)>7:
            yield from walk(event[7])

def main():
    src = SRC.read_text(encoding="utf-8-sig")
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",src))
    game=next((s for s in project["project"][6] if isinstance(s,list)
               and len(s)>1 and s[0]=="Game_events"),None)
    if not game: raise SystemExit("Original Game_events missing")
    entries=[]
    for uid,secondary in GROUPS.items():
        root=find_uid(game[1],uid)
        if root is None: raise SystemExit(f"Missing original activation UID {uid}")
        for event in children(root):
            if not isinstance(event,list) or len(event)<7 or event[0]!=0:
                continue
            item_id=item_parameter(event)
            if item_id not in IDS:continue
            events=[]
            for nested in walk([event]):
                events.append({"uid":nested[4],"conditions":nested[5],"actions":nested[6]})
            entries.append({"item_id":item_id,"secondary":secondary,
                            "source_uid":event[4],"events":events})
            print(f"ORIGINAL ITEM {item_id} SECONDARY={secondary} UID={event[4]} events={len(events)}")
            for nested in events:
                if not nested["actions"]:continue
                print("  ACTION EVENT:",json.dumps(nested,separators=(",",":"))[:4000])
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(entries,separators=(",",":")),encoding="utf-8")
    print(f"Original backpack/tent/stash activation branches: {len(entries)}")

if __name__=="__main__":main()
