#!/usr/bin/env python3
"""Fail closed on invented house containers or misidentified bunker lootboxes.

All source contracts read the authoritative original Construct export.
"""
import json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
raw=(ROOT/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
source=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))["project"]
obj=source[3][796]
animations=obj[7]
assert len(animations)==4,[a[0] for a in animations]
expected=[
 ("lootbox","bunker_lootbox-sheet0.png",(1,1,24,16),(27,1,24,16)),
 ("lootbox_elite","bunker_lootbox-sheet0.png",(1,1,24,16),(27,1,24,16)),
 ("hidden","bunker_lootbox-sheet0.png",(1,19,24,16),(27,19,24,16)),
 ("max","max_box-sheet0.png",(0,0,26,20),(26,0,26,20))
]
for a,(name,sheet,closed,opened) in zip(animations,expected):
 assert a[0]==name and len(a[7])==2
 for fr,rect in zip(a[7],(closed,opened)):
  assert Path(fr[0]).name==sheet,(name,fr[0])
  assert tuple(fr[2:6])==rect,(name,fr[2:6])
  assert (ROOT/"assets/gfx"/sheet).is_file(),sheet
uid=389950912936355
def walk(events):
 for e in events if isinstance(events,list) else []:
  if isinstance(e,list) and len(e)>7 and e[0]==0:
   yield e
   yield from walk(e[7])
game=next(sh[1] for sh in source[6] if sh[0]=="Game_events")
matched=[e for e in walk(game) if e[4]==uid]
assert len(matched)==1
e=matched[0]
# Only player181 COLLISION with box796 and bool1=false opens the source crate.
assert any(c[0]==181 and c[1]==178 and c[9][0][1]==796 for c in e[5])
assert any(c[0]==796 and c[1]==91 and c[9][0]==[10,1] for c in e[5])
assert any(a[0]==796 and a[1]==72 and a[5][0]==[0,[0,1]] for a in e[6])
assert any(a[0]==796 and a[1]==167 and a[2]=="Fade" for a in e[6])
assert any(a[0]==796 and a[1]==161 and a[5]==[[10,1],[3,1]] for a in e[6])
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
assert "rebuild_source_interior_containers();" in main
assert re.search(r"\bupdate_source_interior_containers\(\s*dt\s*\);", main)
assert "source_player_overlaps_world_sprite" in main
# A trash container 314 is not source 796 or a searchable house cupboard.
assert Path(source[3][314][7][0][7][0][0]).name.startswith("obst_trash_container")
print("PASS original bunker lootbox: collision event, 4*2 exact atlas frames, open once / Fade / bool1; no invented house containers")

# Original event 389950912936355 switches frame1 and sets bool1 together.
world=(ROOT/"source/world.cpp").read_text(encoding="utf-8")
scene=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
assert "bool WorldSystem::open_source_bunker_lootbox(" in world
assert "source_bunker_lootbox_closed(box)" in world
assert "placements_[index].source_variable_1=1;" in world
# Scene must use the transactional source path, never bypass its drops.
transaction=(ROOT/"include/minidayz/source_box_simple_drops.hpp").read_text(encoding="utf-8")
assert re.search(r"source_open_simple_box\s*\(\s*world_,idx,player_pos,ground_loot_,world_\.source_random\(\)\)",scene)
assert "world_.open_source_bunker_lootbox(idx)" not in scene
assert "if(!world.open_source_bunker_lootbox(index))return false;" in transaction
assert transaction.index("if(nested && !source_box_default_nested_drops") < transaction.index("if(!world.open_source_bunker_lootbox(index))return false;")
assert transaction.index("if(!world.open_source_bunker_lootbox(index))return false;") < transaction.index("ground=*next;")
assert "placed.source_variable_1=" in world and "p.source_variable_1=" in world
print("PASS: original bunker container frame1 and boolean1 are latched together and replayed from saved frame")
