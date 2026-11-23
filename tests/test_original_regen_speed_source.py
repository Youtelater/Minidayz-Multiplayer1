#!/usr/bin/env python3
"""Root-original Step 4 guard: Regen level ONE must NOT boost AltMove.

The source Player stats event sets PlayerBase193.var5 to 10 only when
perk_regen > 1. Treat this as the actual Construct condition, not a
guess based on the unrelated health-regeneration effect.
"""
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode

project=decode((ROOT/"data.deobfuscated.js").read_bytes())
game=dict(project[6])["Game_events"]

def walk(nodes,ancestors=(),path=()):
    for index,event in enumerate(nodes if isinstance(nodes,list) else []):
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:
            continue
        here=path+(index,)
        yield here,event,ancestors
        if len(event)>7:
            yield from walk(event[7],ancestors+(event,),here)

def source_writer(event):
    return [a for a in event[6]
            if isinstance(a,list) and len(a)>5 and a[:2]==[193,88]
            and a[5] and a[5][0]==[10,5]]

records=list(walk(game))
hits=[(path,e,anc) for path,e,anc in records
      if e[4]==189013257553857]
assert len(hits)==1, "Original Regen speed event changed"
path,e,anc=hits[0]
assert any(c[:2]==[-1,46] and len(c)>9 and
           c[9]==[[11,"perk_regen"],[8,4],[7,[0,1]]]
           for c in e[5] if isinstance(c,list)), (
    "Regen speed must compare perk_regen > 1, not >=1")
writers=source_writer(e)
assert len(writers)==1 and writers[0][5][1]==[7,[0,10]], (
    "Original Regen speed var5=10 action changed")
assert any("Player stats" in str(a[1]) for a in anc if isinstance(a[1],list)), (
    "Regen event no longer under Player stats")
# Ensure the source also resets PlayerBase193.var5 when no Regen
# level-2 condition matches, avoiding a persistent +10 after loading.
resets=[]
for uid in (836845620458514,596810576849508):
    matches=[x for x in records if x[1][4]==uid]
    assert len(matches)==1, f"Source Regen reset UID {uid} changed"
    node=matches[0][1]
    assert len(source_writer(node))==1 and source_writer(node)[0][5][1]==[7,[0,0]]
    resets.append(uid)
native=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
helper=(ROOT/"include/minidayz/source_player_altmove.hpp").read_text(encoding="utf-8")
assert "speed_effects.regen_bonus=minidayz::stage1::source_player_regen_speed_bonus(" in native
assert "source_regen_perk_level > 1 ? 10.0f : 0.0f" in helper
print(f"PASS source Game_events:{'.'.join(map(str,path))} "
      "Regen level2-only speed=+10, else=0; native condition matches")
