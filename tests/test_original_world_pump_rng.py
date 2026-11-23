#!/usr/bin/env python3
"""Guard original water-pump random(100,200) source range and native bounds."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
def walk(nodes):
 for e in nodes if isinstance(nodes,list) else ():
  if isinstance(e,list) and len(e)>=7 and e[0]==0:
   yield e
   if len(e)>7:yield from walk(e[7])
# Find the original per-instance Sprite442 SetInstanceVariable source action
# by behavior, not by cached event UID (which can differ between exports).
original=[(e[4],a) for _,ev in p[6] for e in walk(ev)
 for a in e[6] if isinstance(a,list) and len(a)>5
 and a[0]==442 and a[1]==88
 and "100" in str(a[5]) and "200" in str(a[5])]
assert [uid for uid,_ in original]==[3903561981802149], original
print("ROOT_PUMP_RANDOM_EVENTS", [uid for uid,_ in original])
source=(ROOT/"source/world.cpp").read_text()
assert "100u + source_random_bounded(rng_, 100u)" in source
assert "uniform_int_distribution<std::uint64_t>(100,200)" not in source
print("PASS source pump random(100,200): half-open 100..199 and cross-toolchain deterministic")
