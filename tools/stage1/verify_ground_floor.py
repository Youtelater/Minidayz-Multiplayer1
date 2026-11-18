#!/usr/bin/env python3
"""Compare ALL original Map ground sheets and cells with the 3DS RomFS export.

The original Construct Map holds source ground objects 482/687/688, which
correspond to three distinct map-level terrain textures. No drawn content
is counted as source faithful unless its object AND tile ID match.
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
source_objects = {
    482: "ground_lvl1_tilemap.png",
    687: "ground_lvl2_tilemap.png",
    688: "ground_lvl3_tilemap.png",
}
layout = next(x for x in data[5] if x[0] == "Map")
source_tiles = {}
for key, name in source_objects.items():
    obj = data[3][key]
    assert obj[1] == 16 and Path(obj[6][0]).name == name, (key, name)
    with Image.open(ROOT / "assets/gfx" / name) as im:
        original = im.convert("RGBA")
        assert original.width == original.height and original.width % 2 == 0
        base_art = original.crop((original.width // 2, original.height // 2,
                                  original.width, original.height))
        assert base_art.getbbox() is not None, f"Blank base art: {name}"
        assert len(base_art.getcolors(maxcolors=50000) or []) > 8
        source_tiles[key] = original.size

instances = [inst for layer in layout[6]
    for inst in (layer[14] if len(layer) > 14 and isinstance(layer[14], list) else [])
    if isinstance(inst, list) and len(inst) >= 6 and inst[1] in source_objects]
assert len(instances) >= 4
assert {inst[1] for inst in instances} == set(source_objects)
tile_pixels = int(instances[0][5][1])
visible = set()
expected = []
for inst in instances:
    object_id = inst[1]
    t = inst[0]
    assert int(inst[5][1]) == tile_pixels
    cols, rows, rle = int(t[13][0]), int(t[13][1]), t[13][2]
    cells = []
    for part in str(rle).split(","):
        match = re.fullmatch(r"(\d+)x(-?\d+)|(-?\d+)", part)
        assert match is not None, part
        n = int(match[1] or 1)
        v = int(match[2] if match[2] is not None else match[3])
        cells.extend([v] * n)
        if v != -1:
            tile_id = v & 0x1fffffff
            width, height = source_tiles[object_id]
            assert tile_id < (width // tile_pixels) * (height // tile_pixels)
            visible.add((object_id, tile_id))
    assert len(cells) == cols * rows
    expected.append((float(t[0]), float(t[1]), cols, rows, object_id, cells))

per_page = min(128, max(1, 1024 // tile_pixels) ** 2)
mapping = {key: (i // per_page, i % per_page) for i, key in enumerate(sorted(visible))}
path = ROOT / "romfs/gfx/original_map_ground_layout.bin"
with path.open("rb") as stream:
    header = stream.read(16)
    magic, tw, pages, count = struct.unpack("<4sIII", header)
    assert magic == b"MDG2" and tw == tile_pixels and count == len(expected)
    assert pages == (len(visible) + per_page - 1) // per_page
    total = 0
    for x, y, cols, rows, object_id, cells in expected:
        sx, sy, w, h = struct.unpack("<ffII", stream.read(16))
        assert abs(sx - x) < 0.01 and abs(sy - y) < 0.01
        assert (w, h) == (cols, rows)
        raw = stream.read(4 * cols * rows)
        assert len(raw) == 4 * cols * rows
        packed = struct.unpack("<" + "I" * (cols * rows), raw)
        for original, result in zip(cells, packed):
            if original == -1:
                wanted = 0xffffffff
            else:
                page, frame = mapping[(object_id, original & 0x1fffffff)]
                wanted = (page << 16) | frame
            assert result == wanted, (object_id, original, result, wanted)
        total += len(cells)
    assert stream.read(1) == b""
for page in range(pages):
    atlas = ROOT / "romfs/gfx" / f"original_map_ground_p{page:03}.t3x"
    assert atlas.is_file() and atlas.stat().st_size > 32
for level in range(3):
    name = ("original_map_base_ground.t3x" if level == 0 else
            f"original_map_base_ground_lvl{level + 1}.t3x")
    atlas = ROOT / "romfs/gfx" / name
    assert atlas.is_file() and atlas.stat().st_size > 32, name
header = (ROOT / "include/minidayz/generated/ground_floor_source.hpp").read_text()
assert f"kOriginalFloorBaseTileSize={float(tile_pixels):.1f}f" in header
print(f"Verified all THREE original ground-level images and {len(instances)} Map "
      f"source tilemap instances ({total} cells, {len(visible)} keyed tile frames, "
      f"{pages} atlas pages)")
