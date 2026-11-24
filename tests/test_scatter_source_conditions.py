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
event[2]=True
event[5].append([-1,46,None,0,False,False,False,4,False,[[11,"perk_dao"],[8,0],[7,[0,5]]]])
assert source_level_mask([event])==0x3f
print("PASS source Sprite514 marker (slot 0) and CurrentLevel OR perk_dao event gating")

# World transitions discard old terrain; source SetTile choices now
# consume the same RNG stream as original location generation rather than
# a second, camera-dependent random number generator.
world=(ROOT/"source/world.cpp").read_text(encoding="utf-8")
render=(ROOT/"source/terrain_scatter_render.cpp").read_text(encoding="utf-8")
header=(ROOT/"include/minidayz/world.hpp").read_text(encoding="utf-8")
assert "source_scatter_patches_.clear();" in world
assert "SourceLocationTerrain p(" in world
assert "p.execute()" in world
assert "append(p.scatter,p.scatter_created" in world
program=(ROOT/"include/minidayz/generated/location_terrain_program.hpp").read_text()
assert "kSourceLocationTerrainPaintCount=634" in program
assert "c.level" in program and "c.perk" in program
assert "source_random_bounded(" in world
assert "WorldScatterPatch" in header
assert "world.source_scatter_patches()" in render
assert "std::minstd_rand" not in render
