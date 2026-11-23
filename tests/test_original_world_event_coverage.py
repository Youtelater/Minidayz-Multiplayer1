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
assert len(index)==len(events)
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
             "audit_original_mapgen_dispatch.py","trace_original_mapgen_coordinates.py"):
 code=(ROOT/"tools/stage1"/name).read_text()
 assert "len(node) < 8" not in code
 assert "len(e)<8" not in code
 assert "len(n)>=8" not in code
 assert "len(child)>=8" not in code
print("PASS original events: 22,343 total, 14,560 7-field leaves; 102 mapgen-to-array and 5,747 location actions preserved")
