#!/usr/bin/env python3
"""Verify source item 49: consume, wait five seconds, then award 50 XP."""
import json
import re
from pathlib import Path

root=Path(__file__).resolve().parents[2]
source=(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",source))
def walk(events):
    for e in events if isinstance(events,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e
        if len(e)>7:yield from walk(e[7])
game=next(s for s in project["project"][6] if s[0]=="Game_events")
events=[e for e in walk(game[1]) if e[4]==422420671955084]
assert len(events)==1, "Source Newspapers experience event missing"
event=events[0]
assert any(c[0:2]==[189,112] and
           [7,[0,49]] in c[9] for c in event[5]), "Source item ID changed"
actions=event[6]
consume=[i for i,a in enumerate(actions) if a[0:2]==[189,272]]
wait=[i for i,a in enumerate(actions) if a[0:2]==[-1,54] and
      a[5]==[[0,[0,5]]]]
experience=[i for i,a in enumerate(actions) if a[0:2]==[-1,122] and
      a[5]==[[11,"expierence_points"],[7,[0,50]]]]
assert len(consume)==1 and len(wait)==1 and len(experience)==1 and (
    consume[0]<wait[0]<experience[0]), "Source timed XP ordering changed"
print("Verified original Newspapers(49) consume -> Wait(5) -> +50 XP, UID422420671955084")
