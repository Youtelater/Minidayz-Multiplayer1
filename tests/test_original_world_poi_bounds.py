#!/usr/bin/env python3
"""Root Construct half-open random bounds for initial map-array POIs."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
uid=6938246402369311
def walk(nodes):
 for e in nodes if isinstance(nodes,list) else ():
  if not isinstance(e,list) or len(e)<8:continue
  if e[0]==0:yield e
  if e[0]==0:yield from walk(e[7])
root=next(e for _,events in p[6] for e in walk(events) if e[4]==uid)
expressions={"check_X":set(),"check_Y":set()}
for e in walk(root[7]):
 for a in e[6]:
  if isinstance(a,list) and len(a)>5 and a[:2]==[-1,41] and len(a[5])>=2:
   param=a[5][0]
   if len(param)>1 and param[0]==11 and param[1] in expressions:
    expressions[param[1]].add(repr(a[5][1]))
for key in ("check_X","check_Y"):
 expected="[7, [19, 80, [[19, 95, [[0, 1], [23, 'Map_size_"+("X" if key=="check_X" else "Y")+"']]]]]]]"
 assert expected in expressions[key], (key,len(expressions[key]),list(expressions[key])[:5])
source=(ROOT/"source/world.cpp").read_text()
assert "grid.width() - 1u" in source
assert source.count("max_y - min_y + 1")>=2
assert "const int first_max_y = static_cast<int>(profile.map_size_y) - 1;" in source
assert "const int second_max_y = static_cast<int>(profile.grid_height) - 1;" in source
print("PASS: original floor(random(1,Map_size_X/Y)) bounds preserved; no inclusive last-row/column POI spawn")
