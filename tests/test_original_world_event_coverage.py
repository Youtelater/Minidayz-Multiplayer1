#!/usr/bin/env python3
"""Check all seven-field Construct leaf events remain part of the root world audit.

The original project is not an 8-field-only event encoding: 14,560
seven-field leaves contain real actions. Skipping them silently reports
empty mapgen functions and drops source-created world objects.
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
source=decode((ROOT/"data.deobfuscated.js").read_bytes())
events=[]
def walk(seq):
 for e in seq if isinstance(seq,list) else []:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  events.append(e)
  if len(e)>7 and isinstance(e[7],list):walk(e[7])
for sheet,seq in source[6]:walk(seq)
leaf=[e for e in events if len(e)==7]
assert len(events)==22343,(len(events),len(leaf))
assert len(leaf)==14560,len(leaf)
index={e[4]:e for e in events}
# Original source reuses some event UIDs across duplicated event sheets;
# an event UID is not a globally unique runtime identity. Only the target
# root functions checked below must have one unambiguous definition.
assert len(index)==19246,(len(index),len(events))
def subtree(uid):
 root=index[uid]
 got=[]
 def collect(e):
  got.extend(a for a in e[6] if isinstance(a,list))
  if len(e)>7: 
   for c in e[7]:
    if isinstance(c,list) and len(c)>=7 and c[0]==0:collect(c)
 collect(root)
 return got
assert len(subtree(1694439930581699))==102
assert len(subtree(5017992314331459))==5747
assert any(a[0]==442 and a[1]==88 and
 a[5][1]==[7,[19,82,[[0,100],[0,200]]]]
 for a in index[3903561981802149][6])
for name in ("extract_location_templates.py","trace_source_map_grid_parity.py",
             "audit_original_mapgen_dispatch.py","trace_original_mapgen_coordinates.py",
             "audit_original_road_dispatch.py",
             "audit_original_world_priority.py",
             "audit_original_visual_roles.py"):
 code=(ROOT/"tools/stage1"/name).read_text()
 assert "len(node) < 8" not in code
 assert "len(e)<8" not in code
 assert "len(n)>=8" not in code
 assert "len(child)>=8" not in code
# POI bounds source testing must also walk seven-field source action leaves.
bounds_code=(ROOT/"tests/test_original_world_poi_bounds.py").read_text()
assert "len(e)<8" not in bounds_code
assert "len(e)<7" in bounds_code
# Original Map generation UID3669795404796534 creates Mapgen514 in
# row-major Array518 order with physical transposed axes.
creator=index[3669795404796534]
origins=[a for a in creator[6] if a[:2]==[-1,94] and a[5][0]==[4,514]]
assert len(origins)==1
xc=origins[0][5][2][1]
yc=origins[0][5][3][1]
assert xc[0]==4 and xc[1]==[0,700] and xc[2][0]==6
assert xc[2][2]==[0,1020]
quot=[19,204,[[7,[19,150],[23,"Map_size_X"]]]]
assert xc[2][1]==quot
assert yc[0]==4 and yc[1]==[0,320] and yc[2][0]==6
assert yc[2][1]==[0,1020]
assert "'Map_size_X'" in repr(yc) and "[19, 150]" in repr(yc)
sets={a[5][0][1]:a[5][1] for a in creator[6]
      if a[0]==514 and a[1]==88}
assert sets[1]==[7,quot]
assert "'Map_size_X'" in repr(sets[2])
cpp=(ROOT/"include/minidayz/source_mapgen_geometry.hpp").read_text()
assert "kMapgenStartX=700.0f" in cpp
assert "kMapgenStartY=320.0f" in cpp
assert "kMapgenStride=1020.0f" in cpp
assert "kMapgenCreateEvent=3669795404796534ULL" in cpp
assert "Value::from_number(grid_x);" in (
    ROOT/"source/location_event_runtime.cpp").read_text()
print("PASS original events: 22,343 total, 14,560 7-field leaves; 102 mapgen-to-array and 5,747 location actions preserved")

# Source pump interactions: check both original branches, not just
# the creation-time 100/200 water roll. The high-water branch subtracts
# rounded deficit BEFORE Wait, the low-water branch empties AFTER Wait.
game=next(seq for sheet,seq in source[6] if sheet=="Game_events")
def locate(seq,uid):
    for e in seq:
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        if e[4]==uid:return e
        if len(e)>7:
            result=locate(e[7],uid)
            if result is not None:return result
    return None
high=locate(game,401776724287473)
low=locate(game,5981791044814398)
assert high and low
assert high[5][0][9]==[[10,0],[8,4],[7,[0,25]]]
assert low[5][0][9]==[[10,0],[8,3],[7,[0,25]]]
assert high[6][0][:2]==[442,87] and high[6][0][5][1][1][:2]==[19,80]
assert high[6][2][:2]==[-1,54] and high[6][2][5]==[[0,[0,2]]]
assert high[6][4][:2]==[181,83] and high[6][4][5]==[[10,14],[7,[0,25]]]
assert low[6][0][5][0]==[11,"water_left"]
assert low[6][2][:2]==[-1,54] and low[6][2][5]==[[0,[0,2]]]
assert low[6][4][:2]==[442,88] and low[6][4][5]==[[10,0],[7,[0,0]]]
assert low[6][5][:2]==[181,83] and low[6][5][5]==[[10,14],[7,[23,"water_left"]]]
print("PASS original water pump high/low volume branches and action order")
