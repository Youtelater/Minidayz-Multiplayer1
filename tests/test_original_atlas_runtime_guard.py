#!/usr/bin/env python3
"""Prevent the original full-image atlas regression seen in Manic Emu.

Source metadata is authoritative. Test the generated C++ asset table against
the Construct project's multi-rectangle sprites and tilemap references.
"""
from pathlib import Path
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/stage1"))
from build_original_image_catalog import canonical, source_atlas_metadata

header = (ROOT / "include/minidayz/generated/original_image_catalog.hpp").read_text(encoding="utf-8")
engine = (ROOT / "source/engine.cpp").read_text(encoding="utf-8")
assert "bool is_source_atlas;" in header
assert "if(selected->is_source_atlas)return false;" in engine
assert "draw_source_sprite_fitted" in engine
rects, tilemaps = source_atlas_metadata()
entries = re.findall(r'^\s*\{(".*?"),(".*?"),(true|false)\},\s*$', header, re.M)
assert len(entries) == len(list((ROOT / "assets/gfx").rglob("*.png"))), (
    "Every canonical original PNG needs a catalog entry"
)
guarded = 0
for png_js, _texture_js, flag in entries:
    original = json.loads(png_js)
    name = canonical(original)
    expected = len(rects.get(name, ())) > 1 or name in tilemaps
    assert (flag == "true") == expected, (
        "Wrong original Construct atlas flag for " + original
    )
    guarded += expected
assert guarded >= 850, f"Unexpectedly few original source atlases guarded: {guarded}"
assert "kSpriteFrames" in engine
print(f"PASS: {len(entries)} original files indexed; {guarded} source-confirmed "
      "multi-frame/tilemap atlases cannot silently render as full PNGs")
