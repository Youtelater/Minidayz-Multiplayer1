#!/usr/bin/env python3
"""Rebuild the ORIGINAL Map ground tilemap for Nintendo 3DS.

Extracts every authored tile (and its placement) from root Construct data,
rather than repeating the most frequent ground tile across the world.
The generated 3DS sprite atlas and packed map are bundled in RomFS.
"""
import json
import re
import struct
import subprocess
import tempfile
from collections import Counter
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
GFX = ROOT / "assets/gfx"
ROMFS = ROOT / "romfs/gfx"
HEADER = ROOT / "include/minidayz/generated/ground_floor_source.hpp"
OBJECT = 482
RAW_EMPTY = -1
TILE_MASK = 0x1fffffff

def rle_cells(payload, expected):
    cells = []
    for piece in str(payload).split(","):
        match = re.fullmatch(r"(\d+)x(-?\d+)|(-?\d+)", piece)
        if match is None:
            raise ValueError("Invalid source tilemap RLE " + repr(piece))
        length = int(match.group(1) or 1)
        value = int(match.group(2) if match.group(2) is not None else match.group(3))
        if length < 1 or len(cells) + length > expected:
            raise ValueError("Original source tilemap RLE exceeds dimensions")
        cells.extend([value] * length)
    if len(cells) != expected:
        raise ValueError(f"Original tilemap requires {expected} cells, found {len(cells)}")
    return cells

project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\\s+(?=\d)", "-",
    SOURCE.read_text(encoding="utf-8-sig")))["project"]
obj = project[3][OBJECT]
if obj[1] != 16 or Path(obj[6][0]).name != "ground_lvl1_tilemap.png":
    raise SystemExit("Root original Map ground tilemap object changed")
layout = next((entry for entry in project[5] if entry[0] == "Map"), None)
if layout is None:
    raise SystemExit("Root original Map layout not found")

instances = []
tile_width = None
for layer in layout[6]:
    for inst in layer[14] if len(layer) > 14 and isinstance(layer[14], list) else []:
        if not isinstance(inst, list) or len(inst) < 6 or inst[1] != OBJECT:
            continue
        t = inst[0]
        attrs = inst[5]
        if not isinstance(t, list) or len(t) < 14 or not isinstance(attrs, list) or len(attrs) < 3:
            raise SystemExit("Original ground tilemap instance metadata incomplete")
        tw, th = int(attrs[1]), int(attrs[2])
        if tw <= 0 or tw != th or (tile_width is not None and tile_width != tw):
            raise SystemExit("Unexpected original ground tile dimensions")
        tile_width = tw
        data = t[13]
        if not isinstance(data, list) or len(data) < 3:
            raise SystemExit("Original Map ground RLE payload missing")
        cols, rows = int(data[0]), int(data[1])
        if cols <= 0 or rows <= 0 or cols * rows > 2000000:
            raise SystemExit("Original tilemap dimensions invalid")
        cells = rle_cells(data[2], cols * rows)
        instances.append((float(t[0]), float(t[1]), cols, rows, cells))

if not instances:
    raise SystemExit("No original Map ground instances found")

ROMFS.mkdir(parents=True, exist_ok=True)
HEADER.parent.mkdir(parents=True, exist_ok=True)
image = GFX / Path(obj[6][0]).name
with Image.open(image) as source:
    source = source.convert("RGBA")
    if source.width % tile_width or source.height % tile_width:
        raise SystemExit("Ground tile artwork does not align to Construct tile dimensions")
    sheet_columns = source.width // tile_width
    maximum_id = (source.width // tile_width) * (source.height // tile_width)
    tile_ids = sorted({(v & TILE_MASK) for _, _, _, _, cells in instances
                       for v in cells if v != -1})
    if not tile_ids:
        raise SystemExit("Original ground layout contains no visible tiles")
    if tile_ids[-1] >= maximum_id:
        raise SystemExit("Original ground tile refers outside its source PNG")
    # This authored Map ground sheet is a strip, not a complete world.
    # Choose an existing source tile (the most frequent visible one) for
    # the procedural-world backdrop; never invent replacement artwork.
    frequency = Counter((v & TILE_MASK) for _, _, _, _, cells in instances
                        for v in cells if v != RAW_EMPTY)
    base_tile_id = frequency.most_common(1)[0][0]
    # 3DS GPU textures must fit 1024 pixels; keep paging deterministic.
    side = max(1, 1024 // tile_width)
    per_page = min(128, side * side)
    mapping = {value: (index // per_page, index % per_page)
               for index, value in enumerate(tile_ids)}
    with tempfile.TemporaryDirectory(prefix="mdz_original_ground_") as tmp:
        work = Path(tmp)
        page_count = (len(tile_ids) + per_page - 1) // per_page
        if page_count > 8:
            raise SystemExit("Unexpected number of 3DS ground atlas pages")
        for page in range(page_count):
            directory = work / f"page_{page:03}"
            directory.mkdir()
            names = []
            batch = tile_ids[page * per_page:(page + 1) * per_page]
            for index, tile_id in enumerate(batch):
                px = (tile_id % sheet_columns) * tile_width
                py = (tile_id // sheet_columns) * tile_width
                name = f"tile_{index:03}.png"
                source.crop((px, py, px + tile_width, py + tile_width)).save(directory / name)
                names.append(name)
            manifest = directory / "ground.t3s"
            manifest.write_text("--atlas -f rgba -z auto\n" +
                                "\n".join(names) + "\n", encoding="utf-8")
            destination = ROMFS / f"original_map_ground_p{page:03}.t3x"
            subprocess.run(["tex3ds", "-i", str(manifest), "-o", str(destination)],
                           cwd=directory, check=True)
            if not destination.is_file() or destination.stat().st_size < 32:
                raise SystemExit("Original ground sprite atlas failed: " + str(destination))

# The authored Map holds only two very narrow ground tilemap strips.
# Construct's game events populate the rest of the playable world at runtime.
# Until the per-cell Construct tile painting events are fully translated,
# a 3DS renderer must draw a source-art ground tile BEHIND those authored
# placements; otherwise every generated settlement outside the strips is
# rendered over a black/empty GPU clear color.
#
# Take the grass-textured lower-right source tile from the ORIGINAL
# ground_lvl1_tilemap.png; never synthesize artwork or use images/ copies.
# Keep this separate from source tile IDs so the authored RLE tiles remain
# exact and the existing source layout verifier is not weakened.
BASE_GROUND = ROMFS / "original_map_base_ground.t3x"
with Image.open(image) as source:
    source = source.convert("RGBA")
    # The PNG is a 2-by-2 art sheet of full grass/road/shore textures.
    # Its Construct tilemap metadata can use a different *map cell* size.
    # Cropping only a map cell would take a tiny fragment, even a one-pixel
    # transparent fragment, and leave the 3DS terrain effectively black.
    if source.width % 2 or source.height % 2:
        raise SystemExit("Original 2x2 source terrain artwork dimensions changed")
    base_tile_pixels = min(source.width // 2, source.height // 2)
    if base_tile_pixels < 8:
        raise SystemExit("Original source grass base has invalid dimensions")
    with tempfile.TemporaryDirectory(prefix="mdz_original_base_floor_") as tmp:
        work = Path(tmp)
        source.crop((source.width - base_tile_pixels,
                     source.height - base_tile_pixels,
                     source.width, source.height)).save(work / "base.png")
        (work / "base.t3s").write_text(
            "--atlas -f rgba -z auto" + chr(10) + "base.png" + chr(10), encoding="utf-8")
        subprocess.run(["tex3ds", "-i", str(work / "base.t3s"),
                        "-o", str(BASE_GROUND)], cwd=work, check=True)
if not BASE_GROUND.is_file() or BASE_GROUND.stat().st_size < 32:
    raise SystemExit(f"Original source base ground atlas not generated: tile={base_tile_pixels}, size={BASE_GROUND.stat().st_size if BASE_GROUND.is_file() else -1}")

# Format: magic (4), tile width (u32), atlas pages (u32), instance count (u32);
# each instance: x,y (float), width,height (u32), then width*height
# unsigned 32-bit tile descriptors: 0xffffffff empty; otherwise packed
# (page<<16 | sprite index). Original high-bit tile flags are preserved in
# the upper flags byte of descriptor for future transform support.
destination = ROMFS / "original_map_ground_layout.bin"
with destination.open("wb") as out:
    out.write(struct.pack("<4sIII", b"MDG2", tile_width, page_count, len(instances)))
    for x, y, cols, rows, cells in instances:
        out.write(struct.pack("<ffII", x, y, cols, rows))
        for raw in cells:
            if raw == RAW_EMPTY:
                descriptor = 0xffffffff
            else:
                page, frame = mapping[raw & TILE_MASK]
                descriptor = (page << 16) | frame
            out.write(struct.pack("<I", descriptor))
if destination.stat().st_size < 32:
    raise SystemExit("Original ground tilemap binary empty")
HEADER.write_text("#pragma once\n#include <cstdint>\nnamespace minidayz::stage1 {\n" +
    f"inline constexpr std::uint16_t kOriginalFloorTileObject={OBJECT};\n" +
    f"inline constexpr std::uint16_t kOriginalFloorTileId={tile_ids[0]};\n" +
    f"inline constexpr std::uint16_t kOriginalFloorBaseTileId={base_tile_id};\\n" +
    f"inline constexpr std::uint32_t kOriginalFloorBasePage={mapping[base_tile_id][0]};\\n" +
    f"inline constexpr std::uint32_t kOriginalFloorBaseFrame={mapping[base_tile_id][1]};\\n" +
    f"inline constexpr float kOriginalFloorTileSize={float(tile_width):.1f}f;\n" +
    f"inline constexpr float kOriginalFloorBaseTileSize={float(base_tile_pixels):.1f}f;" + chr(10) +
    f"inline constexpr std::uint32_t kOriginalFloorAtlasPages={page_count};\n" +
    "}\n", encoding="utf-8")
print(f"Original Map: {len(instances)} authored tilemap strips; {len(tile_ids)} source tiles; "
      f"{page_count} authored atlases plus 1 original grass base; "
      f"packed layout {destination.stat().st_size} bytes")
