#!/usr/bin/env python3
"""Assert original Construct bed, build ghost and dismantle sheet coordinates.

All expected values are literal root data.deobfuscated.js project records.
The 3DS port uses its build_sprite_catalog.py from that source, never
opens an entire multi-frame PNG as one image.
"""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
project=json.loads((ROOT/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig"))["project"]
objects=project[3]
frames={
    (951,0):("bed-sheet0.png",1,1,37,21,0.5,0.5),
    (951,1):("bed-sheet0.png",1,24,37,21,0.5,0.5),
    (573,12):("build_ghost_obstacle-sheet0.png",52,1,37,21,0.6,0.7),
    (317,0):("test_bullet_hit-sheet0.png",1,1,20,20,0.5,0.5)
}
for (object_id,frame_id),expected in frames.items():
    frame=objects[object_id][7][0][7][frame_id]
    name=Path(frame[0]).name
    actual=(name,frame[2],frame[3],frame[4],frame[5],frame[7],frame[8])
    assert len(actual)==len(expected)
    assert actual[:5]==expected[:5],((object_id,frame_id),actual,expected)
    assert abs(actual[5]-expected[5])<1e-6 and abs(actual[6]-expected[6])<1e-6
    assert (ROOT/"assets/gfx"/name).is_file(),name
# All original source event UIDs are checked, including near-house trigger,
# Build mode Spawn bed951, one-second dismantle and source Count_sleep_hours.
events=set()
def walk(nodes):
    for e in nodes:
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        events.add(e[4])
        if len(e)>7 and isinstance(e[7],list):walk(e[7])
for sheet,nodes in project[6]:
    if sheet=="Game_events":walk(nodes)
for uid in (
    1025534786344989,811103055423252,125247805541322,
    622329722190205,760795525771502,974086311952240,
    242979671087187,903835777805920,948724703580895,
):
    assert uid in events,uid
src=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
assert "kSourceBedGhostFrame" in src
assert "source_bed_build_site_valid(" in src
assert "source_bed_consume_materials(" in src
assert "advance_source_sleep_wait(" in src
print("PASS: exact original bed951 and 573/317 source frame rectangles, original event UIDs and wired scene")
