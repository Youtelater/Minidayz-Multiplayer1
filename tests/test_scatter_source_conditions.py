#!/usr/bin/env python3
"""Source-backed checks for procedural terrain marker and level selection."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from build_procedural_environment import source_marker_codes, source_level_mask
c=[
 [514,71,None,0,False,False,False,1,False,[[10,1],[8,0],[7,[0,15]]]],
 [514,71,None,0,False,False,False,2,False,[[10,2],[8,0],[7,[0,14]]]],
 [514,71,None,0,False,False,False,3,False,[[10,0],[8,0],[7,[0,3]]]],
]
assert source_marker_codes(c)==[3]
event=[0,None,False,None,1,[
 [-1,46,None,0,False,False,False,2,False,[[11,"CurrentLevel"],[8,5],[7,[0,4]]]]
 ],[],[]]
assert source_level_mask([event])==0b110000
assert source_level_mask([])==0x3f
event[5][0][9][1]=[8,0]
assert source_level_mask([event])==(1<<4)
print("PASS source Sprite514 marker (slot 0) and CurrentLevel event gating")

# Terrain patches from a previous level may share the same world seed and
# marker ID, but must NEVER reuse the cached painted frames after a transition.
header=(ROOT/"include/minidayz/engine.hpp").read_text(encoding="utf-8")
render=(ROOT/"source/terrain_scatter_render.cpp").read_text(encoding="utf-8")
assert "std::uint8_t level{255};" in header
assert "candidate.level==world.generation_level()" in render
assert "patch->level=world.generation_level()" in render
