#!/usr/bin/env python3
"""Check every packed 3DS terrain cell against the root original Map source.

Requires make stage1-assets. This verifies the full ground layout including
empty cells, atlas frame indices and original sprite pixels' source references.
It does not test Construct per-tile flipping or actual 3DS rendering.
"""
import json
import re
import struct
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
data = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
    (ROOT / "data/stage1/construct_project_parsed.json").read_text(
        encoding="utf-8-sig")))["project"]
ground_obj = data[3][482]
assert ground_obj[1] == 16
layout = next(layout for layout in data[5] if layout[0] == "Map")
instances = [inst for layer in layout[6]
    for inst in (layer[14] if len(layer) > 14 and isinstance(layer[14], list) else [])
    if isinstance(inst, list) and len(inst) > 5 and inst[1] == 482]
assert instances
tile_pixels = int(instances[0][5][1])
with Image.open(ROOT / "assets/gfx" / Path(ground_obj[6][0]).name) as img:
    tile_count = (img.width // tile_pixels) * (img.height // tile_pixels)
visible_ids = set()
source_data = []
for inst in instances:
    t = inst[0]
    cols, rows, rle = int(t[13][0]), int(t[13][1]), t[13][2]
    cells = []
    for part in str(rle).split(","):
        match = re.fullmatch(r"(\d+)x(-?\d+)|(-?\d+)", part)
        assert match is not None, part
        n = int(match[1] or 1)
        v = int(match[2] if match[2] is not None else match[3])
        cells.extend([v] * n)
        if v != -1:
            visible_ids.add(v & 0x1fffffff)
    assert len(cells) == cols * rows
    source_data.append((float(t[0]), float(t[1]), cols, rows, cells))
assert visible_ids and max(visible_ids) < tile_count
per_page = min(128, max(1, 1024 // tile_pixels) ** 2)
mapping = {value: (index // per_page, index % per_page)
           for index, value in enumerate(sorted(visible_ids))}
path = ROOT / "romfs/gfx/original_map_ground_layout.bin"
with path.open("rb") as file:
    header = file.read(16)
    magic, tw, page_count, instance_count = struct.unpack("<4sIII", header)
    assert magic == b"MDG2" and tw == tile_pixels
    assert instance_count == len(instances)
    assert page_count == (len(visible_ids) + per_page - 1) // per_page
    total = 0
    for expected_x, expected_y, cols, rows, cells in source_data:
        x, y, w, h = struct.unpack("<ffII", file.read(16))
        assert (x, y, w, h) == (expected_x, expected_y, cols, rows)
        contents = file.read(4 * cols * rows)
        assert len(contents) == 4 * cols * rows
        values = struct.unpack("<" + "I" * (cols * rows), contents)
        for original, packed in zip(cells, values):
            wanted = (0xffffffff if original == -1 else
                      (mapping[original & 0x1fffffff][0] << 16) |
                      mapping[original & 0x1fffffff][1])
            assert packed == wanted
        total += len(cells)
    assert file.read(1) == b"", "Unexpected data after last original tile"
for page in range(page_count):
    atlas = ROOT / "romfs/gfx" / f"original_map_ground_p{page:03}.t3x"
    assert atlas.is_file() and atlas.stat().st_size > 32
print(f"Verified original ground: {len(instances)} source instances, "
      f"{total} exact cells, {len(visible_ids)} tile images, {page_count} atlases")
