#!/usr/bin/env python3
"""Verify exact root MiniDAYZ Sprite519 minimap frame event program."""
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())

def walk(seq):
    for e in seq if isinstance(seq,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:
            continue
        yield e
        if len(e)>7:
            yield from walk(e[7])

nodes=[e for _,sheet in project[6] for e in walk(sheet)]
f=[e for e in nodes if e[4]==6302523114231127]
assert len(f)==1
root=f[0]
rules=[]
def visit(e,parents=()):
    for a in e[6]:
        if a[:2]==[519,72]:
            groups=[x for x in (*parents,e) if x[5]]
            marker_types=tuple(sorted({
                c[9][2][1][1]
                for g in groups for c in g[5]
                if c[:2]==[514,71] and c[9][0]==[10,0]
                and c[9][1]==[8,0] and
                c[9][2][:1]==[7] and c[9][2][1][:1]==[0]
            }))
            neighbor_groups=[
                g for g in groups
                if any(c[:2]==[518,203] for c in g[5])
            ]
            rules.append((e[4],a[5][0][1][1],
                          marker_types,len(neighbor_groups)))
    for c in e[7] if len(e)>7 else ():
        if isinstance(c,list) and c[0]==0:
            visit(c,(*parents,e))
visit(root)
assert len(rules)==42,len(rules)
assert all(0<=frame<39 for _,frame,_,_ in rules)
# The source's 42 authored SetFrame actions include 9 nested
# road-neighbor overrides for road13/road12; the native helper must
# implement those original Array518 predicates rather than drawing a bar.
assert sum(v>0 for _,_,_,v in rules)==9,rules
single={
    (code,frame)
    for _,frame,types,n in rules if n==0
    for code in types
}
expected={
    (50,19),(51,20),(108,36),(109,37),(67,38),
    (52,29),(23,14),(3,7),(32,26),(5,5),
    (22,27),(70,30),(11,8),(14,3),(15,9),
    (20,7),(30,17),(31,16),(53,21),(54,22),
    (55,23),(56,24),(102,31),(103,32),(104,33),
    (105,4),(106,34),(110,34),(107,35)
}
assert expected<=single,(expected-single)
assert project[3][519][7][0][7][0][4:6]==[20,20]
assert len(project[3][519][7][0][7])==39

cpp=(ROOT/"include/minidayz/source_minimap_frames.hpp").read_text()
for n in ("kOriginalMinimapSprite=519",
          "kOriginalMinimapFrameCount=39",
          "original_minimap_road_neighbor",
          "original_minimap_frame("):
    assert n in cpp,n
source=(ROOT/"source/main.cpp").read_text()
# Preserve the exact original 519 frame/event catalog for source parity,
# even when the user chooses a custom map-only 3DS presentation.
assert "original_minimap_frame(" in cpp
assert "renderer.draw_source_sprite(placement->object" in source
assert "classify_color=[]" not in source
map_block=source.split("    void render_source_map(",1)[1].split(
    "    void render_bottom(",1)[0]
assert "original_minimap_frame(" not in map_block
assert "marker_cell_kind(" in map_block
assert "source_cell_rect(" in map_block
print("PASS original Sprite519 39-frame atlas, all 42 root minimap "
      "SetFrame events including 9 raw road neighbor overrides")
