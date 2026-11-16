#!/usr/bin/env python3
"""Guard original XP and hamster/camel perk source values against drift."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
p = ROOT / "data/stage1/construct_project_parsed.json"
source = p.read_text(encoding="utf-8-sig")
project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\\s+(?=\\d)", "-", source))
sheets = project["project"][6]
game = next(x for x in sheets if isinstance(x,list) and x and x[0]=="Game_events")
defaults = {x[1]:x[3] for x in game[1] if isinstance(x,list) and len(x)>3 and x[0]==1}
for name,initial in (("perk_cost",100),("expierence_points",0),
                     ("perks_learnt_count",0),("perk_hamster",0),("perk_camel",0)):
    assert defaults.get(name)==initial,(name,defaults.get(name))

def children(e):
    return e[7] if len(e)>7 and isinstance(e[7],list) else []
def walk(events):
    for e in events if isinstance(events,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e
        yield from walk(children(e))
by_uid = {}
for e in walk(game[1]):by_uid.setdefault(e[4], []).append(e)
cost = by_uid.get(454712397231976,[])
assert any(a[0:2] == [-1,41] and a[5] == [
    [11,"perk_cost"],
    [7,[4,[4,[23,"perk_cost"],[0,100]],[6,[0,20],[23,"perks_learnt_count"]]]]
    ] for e in cost for a in e[6]),"Original increasing perk cost changed"
for name, uid, level in (("perk_camel",774754508115784,1),
                         ("perk_camel",784538107764217,2),
                         ("perk_hamster",109489764621466,1),
                         ("perk_hamster",136357405231692,2)):
    events = by_uid.get(uid,[])
    assert any(a[0:2]==[-1,41] and a[5] ==
               [[11,name],[7,[0,level]]] for e in events for a in e[6]), (
               "Original perk level change missing",name,level,uid)
print("Original perk globals, 2 levels and formula verified")
