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
for uid,item_id,seconds,experience in (
    (422420671955084,49,5,50),
    (853523169384096,116,10,200)):
    events=[e for e in walk(game[1]) if e[4]==uid]
    assert len(events)==1, f"Source XP event missing: {uid}"
    event=events[0]
    assert any(c[0:2]==[189,112] and [7,[0,item_id]] in c[9]
               for c in event[5]), f"Source item ID changed: {uid}"
    actions=event[6]
    consume=[i for i,a in enumerate(actions) if a[0:2]==[189,272]]
    wait=[i for i,a in enumerate(actions) if a[0:2]==[-1,54] and
          a[5]==[[0,[0,seconds]]]]
    awards=[i for i,a in enumerate(actions) if a[0:2]==[-1,122] and
          a[5]==[[11,"expierence_points"],[7,[0,experience]]]]
    assert len(consume)==1 and len(wait)==1 and len(awards)==1 and (
        consume[0]<wait[0]<awards[0]), f"Original timed XP ordering changed: {uid}"
    print(f"Verified original item {item_id} consume -> Wait({seconds}) -> "
          f"+{experience} XP at source UID {uid}")
