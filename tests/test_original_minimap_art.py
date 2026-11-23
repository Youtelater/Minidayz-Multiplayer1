#!/usr/bin/env python3
"""Prevent the screenshot-proven error of stretching original icon atlases.

The original MiniDAYZ minimap PNGs include multiple independent frames.
Only actual source sprite frames may represent player/location markers.
"""
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
for file in ("minimap_tile-sheet0.png","minimap_mark-sheet0.png",
             "minimap_player-sheet0.png"):
    assert (ROOT/"assets/gfx"/file).is_file(),file
    assert f'draw_original_image("{file}"' not in main,file
assert '"minimap_mark-sheet0.png"' not in main
assert '"minimap_player-sheet0.png"' not in main
assert "minimap_mark_sheet0_" not in main  # don't repeat first unknown icon per POI
assert 'source_cell_rect(' in main
assert 'case 99: return CellKind::gas_station;' in (ROOT/"include/minidayz/source_map_overlay.hpp").read_text()
assert "minimap_player_sheet0_" in main
assert "renderer.draw_source_sprite_fitted(" in main
assert "render_source_map" in main
assert "SEED %08X" in main and "LVL %u" in main
# Inspect the actual user-recommended root multiplayer seed shim. It wraps
# the original generator, hashes the generated Array/map and overrides
# Math.random only during its seeded multiplayer generation window.
# That is diagnostics / reproducibility, NOT a replacement UI atlas.
import re
mp=(ROOT/"mp_worldseed.js").read_text(encoding="utf-8")
mp=re.sub(r"\\u([0-9a-fA-F]{4})",
          lambda m:chr(int(m.group(1),16)),mp)
for fn in ("generate_array_locations","generate_array_roads_1",
           "generate_array_roads_2","generate_array_tomap",
           "generate_mapgen_toelements","generate_minimap"):
    assert fn in mp,fn
assert "arrayHash" in mp and "mapHash" in mp and 'Math["random"]' in mp
print("PASS: source minimap atlas frames instead of crushed full-sheet icon images")
