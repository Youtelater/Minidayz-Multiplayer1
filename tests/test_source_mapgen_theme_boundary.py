#!/usr/bin/env python3
"""Verify transposed original Mapgen logical Y -> world X theme boundary."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
engine=(ROOT/"source/engine.cpp").read_text(encoding="utf-8")
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
assert "source_second_half_x" in main and "mapgen_hotspot(0,18).x" in main
assert "source_second_half_y" not in main
assert "const float second_half_x=(source_level==5 && grid.height()==33)" in engine
assert "source_level,tile_world_x,second_half_x" in engine
assert "source_level,world_tile_x,second_half_x" in engine
assert "source_level,map.x+(static_cast<float>(col)+0.5f)*tile" in engine
assert "source_level,map.y+(static_cast<float>(row)+0.5f)*tile" not in engine
assert "source_level,world_tile_y,second_half_y" not in engine
# Original mapgen X=700+1020*ArrayY: row18 starts at worldX=19060.
assert 700+18*1020==19060
print("PASS source 16x33 split map: worldX ground and asphalt themes at original Mapgen row18")
