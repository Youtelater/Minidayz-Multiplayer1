#!/usr/bin/env python3
"""Original Construct mobile car distance comparator vs native A adaptation."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
def walk(nodes):
    for e in nodes if isinstance(nodes,list) else ():
        if isinstance(e,list) and len(e)>=7 and e[0]==0:
            yield e
            if len(e)>7:yield from walk(e[7])
events=[e for sheet,src in project[6] if sheet=="Game_events"
        for e in walk(src) if e[4]==711142196261088]
assert len(events)==1, "Original car mobile proximity event changed"
assert any(c[:2]==[-1,51] and len(c)>9 and c[9][1]==[8,3] and
           c[9][2]==[7,[0,100]] and c[9][0][:2]==[7,[19,79]]
           for c in events[0][5])
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
generator=(ROOT/"tools/stage2/build_original_car_trunk_loot.py").read_text(encoding="utf-8")
assert "distance=minidayz::items::kOriginalCarTrunkInteractionRange" in main
assert "source distance < 50" not in main
assert "kOriginalCarTrunkInteractionRange=100.0f" in generator
print("PASS root mobile car distance<=100 uses original comparison/extent in 3DS")
