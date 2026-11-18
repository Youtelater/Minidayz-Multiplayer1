#!/usr/bin/env python3
"""Regression test: original source sprite names must never alias Tex3DS atlases."""
from __future__ import annotations

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
builder = ROOT / "tools/stage1/build_sprite_catalog.py"
spec = importlib.util.spec_from_file_location("sprite_catalog", builder)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

images = sorted((ROOT / "assets/gfx").glob("*.png"))
assert images
owners = {}
for image in images:
    key = module.ident(image.name)
    if key in owners and owners[key] != image.name:
        raise AssertionError(
            f"Sprite atlas {key} would overwrite {owners[key]} with {image.name}")
    owners[key] = image.name

# These different original Cyrillic names used to collapse to identical
# underscore-only names, silently replacing their animation sheets.
assert module.ident("верх1.png") != module.ident("лево1.png")
assert module.ident("перс1.png") != module.ident("верх1.png")
assert module.ident("a-b.png") != module.ident("a_b.png")
print(f"Verified collision-free atlas names for {len(images)} original PNGs")
