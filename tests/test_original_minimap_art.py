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
assert "minimap_mark_sheet0_" in main
assert "minimap_player_sheet0_" in main
assert "renderer.draw_source_sprite_fitted(" in main
assert "render_source_map" in main
print("PASS: source minimap atlas frames instead of crushed full-sheet icon images")
