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
# Source Map has three distinct ground tilemap objects, and its generation
# events select between them for different CurrentLevel branches.
SOURCE_GROUND = {482: "ground_lvl1_tilemap.png",
                 687: "ground_lvl2_tilemap.png",
                 688: "ground_lvl3_tilemap.png",
                 776: "ground_lvl4_tilemap.png",
                 980: "ground_lvl5_tilemap.png",
                 1138: "ground_lvl6_tilemap.png",
                 1139: "ground_lvl7_tilemap.png"}
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

project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
    SOURCE.read_text(encoding="utf-8-sig")))["project"]
obj = project[3][OBJECT]
for object_id, filename in SOURCE_GROUND.items():
    definition = project[3][object_id]
    if definition[1] != 16 or Path(definition[6][0]).name != filename:
        raise SystemExit(f"Original ground tile object {object_id} changed")
layout = next((entry for entry in project[5] if entry[0] == "Map"), None)
if layout is None:
    raise SystemExit("Root original Map layout not found")

instances = []
tile_width = None
for layer in layout[6]:
    for inst in layer[14] if len(layer) > 14 and isinstance(layer[14], list) else []:
        if not isinstance(inst, list) or len(inst) < 6 or inst[1] not in SOURCE_GROUND:
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
        instances.append((float(t[0]), float(t[1]), cols, rows, cells, inst[1]))

if not instances:
    raise SystemExit("No original Map ground instances found")

ROMFS.mkdir(parents=True, exist_ok=True)
HEADER.parent.mkdir(parents=True, exist_ok=True)
# All three ORIGINAL source ground sheets share the Construct 60px cell
# dimensions, but their frame IDs refer to separate textures. Keep each
# (object, tile ID) distinct or level 2/3 ground will silently show level 1.
images = {}
for object_id, filename in SOURCE_GROUND.items():
    with Image.open(GFX / filename) as image_source:
        im = image_source.convert("RGBA")
        if im.width % tile_width or im.height % tile_width:
            raise SystemExit(f"Source tile sheet does not align: {filename}")
        images[object_id] = im.copy()

sheet_columns = {key: im.width // tile_width for key, im in images.items()}
# Runtime Construct tile-paint actions can select tiles absent from the
# narrow pre-authored Map strips. Export EVERY original source tile ID for
# all seven level themes, so those events can use exact artwork later.
# Keep (object ID, tile ID) distinct: identical numeric IDs across level
# sheets are different pixels and must never share an atlas reference.
tile_keys = sorted((object_id, tile_id)
                   for object_id, im in images.items()
                   for tile_id in range((im.width // tile_width) *
                                        (im.height // tile_width)))
if not tile_keys:
    raise SystemExit("Original Map contains no ground tile art")
for object_id, tile_id in tile_keys:
    im = images[object_id]
    count = (im.width // tile_width) * (im.height // tile_width)
    if tile_id >= count:
        raise SystemExit(f"Ground tile {object_id}:{tile_id} outside source art")

# Retain exact authored source IDs and original level-one fallback metadata.
frequency = Counter((raw & TILE_MASK)
                    for _, _, _, _, cells, object_id in instances
                    if object_id == OBJECT for raw in cells if raw != RAW_EMPTY)
base_tile_id = frequency.most_common(1)[0][0]
side = max(1, 1024 // tile_width)
per_page = min(128, side * side)
mapping = {key: (index // per_page, index % per_page)
           for index, key in enumerate(tile_keys)}
with tempfile.TemporaryDirectory(prefix="mdz_original_ground_") as tmp:
    work = Path(tmp)
    page_count = (len(tile_keys) + per_page - 1) // per_page
    if page_count > 8:
        raise SystemExit("Unexpected number of 3DS ground atlas pages")
    for page in range(page_count):
        directory = work / f"page_{page:03}"
        directory.mkdir()
        names = []
        batch = tile_keys[page * per_page:(page + 1) * per_page]
        for index, (object_id, tile_id) in enumerate(batch):
            im = images[object_id]
            px = (tile_id % sheet_columns[object_id]) * tile_width
            py = (tile_id // sheet_columns[object_id]) * tile_width
            name = f"tile_{index:03}.png"
            im.crop((px, py, px + tile_width, py + tile_width)).save(directory / name)
            names.append(name)
        manifest = directory / "ground.t3s"
        manifest.write_text("--atlas -f rgba -z auto" + chr(10) +
                            chr(10).join(names) + chr(10), encoding="utf-8")
        destination = ROMFS / f"original_map_ground_p{page:03}.t3x"
        subprocess.run(["tex3ds", "-i", str(manifest), "-o", str(destination)],
                       cwd=directory, check=True)
        if not destination.is_file() or destination.stat().st_size < 32:
            raise SystemExit("Original ground sprite atlas failed: " + str(destination))

# The root original Map's TWO Sprite482 tilemap instances have 5,975
# authored cells of tile ID 6 each (plus only two transition cells 21/22).
# Other root Map ground sheets also place ID 6 when occupied. Therefore
# use the same original *source-authored repeating interior tile* for the
# 3DS fallback where the root layout contains no floor cell. Tile ID 4
# is an ORIGINAL transition graphic, NOT a plain background. Tiling it
# made the user's screenshots show a blatant checkerboard of dark squares.
# No original PNG is recolored, synthesized or renamed.
source_base_tile = Counter((raw & TILE_MASK)
    for _, _, _, _, cells, object_id in instances if object_id == OBJECT
    for raw in cells if raw != RAW_EMPTY).most_common(1)[0][0]
if source_base_tile != 6:
    raise SystemExit(f"Root original Map floor tile changed: {source_base_tile}")
for object_id in SOURCE_GROUND:
    authored=Counter((raw & TILE_MASK)
        for _, _, _, _, cells, oid in instances if oid==object_id
        for raw in cells if raw != RAW_EMPTY)
    if authored and authored.most_common(1)[0][0]!=source_base_tile:
        raise SystemExit(
            f"Original ground theme {object_id} has different interior tile")
base_tile_pixels=tile_width
for level, object_id in enumerate(SOURCE_GROUND):
    source = images[object_id]
    if sheet_columns[object_id] != 5 or source.height < tile_width:
        raise SystemExit(f"Unexpected original 5-column ground atlas {object_id}")
    base_id = source_base_tile
    # Retain exact root-authored Map interior cell, not transition tile4.
    # The original source art must contain the selected 60x60 background.
    plain_x = (base_id % sheet_columns[object_id]) * tile_width
    source_plain = source.crop((plain_x,0,plain_x + tile_width,tile_width))
    if source_plain.getbbox() is None:
        raise SystemExit(f"Original source background tile missing: {object_id}:{base_id}")
    x = (base_id % sheet_columns[object_id]) * tile_width
    y = (base_id // sheet_columns[object_id]) * tile_width
    if level == 0:
        destination_base = ROMFS / "original_map_base_ground.t3x"
    else:
        destination_base = ROMFS / f"original_map_base_ground_lvl{level + 1}.t3x"
    with tempfile.TemporaryDirectory(prefix=f"mdz_base_{level}_") as tmp:
        work = Path(tmp)
        source.crop((x, y, x + tile_width, y + tile_width)).save(work / "base.png")
        (work / "base.t3s").write_text(
            "--atlas -f rgba -z auto" + chr(10) + "base.png" + chr(10),
            encoding="utf-8")
        subprocess.run(["tex3ds", "-i", str(work / "base.t3s"),
                        "-o", str(destination_base)], cwd=work, check=True)
    if not destination_base.is_file() or destination_base.stat().st_size < 32:
        raise SystemExit(f"Original source level {level} ground atlas missing")
# Recover the authored base pavement from the two straight-road paint events.
# Other location-loader writes can subsequently reshape the tile edges.
road_actions = {358622687629146, 532767105758898}
road_tiles = {}
def collect_road_tiles(events):
    for event in events:
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        for action in event[6]:
            if len(action) < 6 or action[3] not in road_actions:
                continue
            if action[:2] != [1066, 202] or action[5][4][:1] != [0]:
                raise ValueError("Original straight-road paint command changed")
            literal = action[5][4][1]
            if literal[0] != 0:
                raise ValueError("Original straight-road pavement is no longer literal")
            road_tiles[action[3]] = literal[1]
        if len(event) > 7:
            collect_road_tiles(event[7])

collect_road_tiles(dict(project[6])["Game_events"])
if road_tiles.keys() != road_actions or len(set(road_tiles.values())) != 1:
    raise ValueError(f"Original straight-road tile mismatch: {road_tiles}")
road_tile = next(iter(road_tiles.values()))
for level, object_id in enumerate(SOURCE_GROUND, start=1):
    source = images[object_id]
    x = road_tile % sheet_columns[object_id] * tile_width
    y = road_tile // sheet_columns[object_id] * tile_width
    stone_tile = source.crop((x, y, x + tile_width, y + tile_width))
    if stone_tile.getbbox() is None:
        raise SystemExit(f"Original road artwork missing in ground level {level}")
    with tempfile.TemporaryDirectory(prefix=f"mdz_road_{level}_") as tmp:
        work = Path(tmp)
        stone_tile.save(work / "road.png")
        (work / "road.t3s").write_text(
            "--atlas -f rgba -z auto" + chr(10) + "road.png" + chr(10),
            encoding="utf-8")
        road_atlas = ROMFS / f"original_map_road_lvl{level}.t3x"
        subprocess.run(["tex3ds", "-i", str(work / "road.t3s"),
                        "-o", str(road_atlas)], cwd=work, check=True)
        if not road_atlas.is_file() or road_atlas.stat().st_size < 32:
            raise SystemExit(f"Original road texture {level} was not exported")

BASE_GROUND = ROMFS / "original_map_base_ground.t3x"

# Format: magic (4), tile width (u32), atlas pages (u32), instance count (u32);
# each instance: x,y (float), width,height (u32), then width*height
# unsigned 32-bit tile descriptors: 0xffffffff empty; otherwise packed
# (source flip bits | page<<16 | sprite index). Original horizontal,
# vertical and diagonal tile transformations remain available at runtime.
destination = ROMFS / "original_map_ground_layout.bin"
with destination.open("wb") as out:
    out.write(struct.pack("<4sIII", b"MDG3", tile_width, page_count, len(instances)))
    for x, y, cols, rows, cells, object_id in instances:
        out.write(struct.pack("<ffIII", x, y, cols, rows, object_id))
        for raw in cells:
            if raw == RAW_EMPTY:
                descriptor = 0xffffffff
            else:
                page, frame = mapping[(object_id, raw & TILE_MASK)]
                descriptor = (raw & 0xe0000000) | (page << 16) | frame
            out.write(struct.pack("<I", descriptor))
if destination.stat().st_size < 32:
    raise SystemExit("Original ground tilemap binary empty")
HEADER.write_text("#pragma once\n#include <cstdint>\nnamespace minidayz::stage1 {\n" +
    f"inline constexpr std::uint16_t kOriginalFloorTileObject={OBJECT};\n" +
    f"inline constexpr std::uint16_t kOriginalFloorTileId={min(key[1] for key in tile_keys if key[0] == OBJECT)};\n" +
    f"inline constexpr std::uint16_t kOriginalFloorBaseTileId={base_tile_id};\n" +
    f"inline constexpr std::uint32_t kOriginalFloorBasePage={mapping[(OBJECT, base_tile_id)][0]};\n" +
    f"inline constexpr std::uint32_t kOriginalFloorBaseFrame={mapping[(OBJECT, base_tile_id)][1]};\n" +
    f"inline constexpr float kOriginalFloorTileSize={float(tile_width):.1f}f;\n" +
    f"inline constexpr float kOriginalFloorBaseTileSize={float(base_tile_pixels):.1f}f;" + chr(10) +
    f"inline constexpr std::uint32_t kOriginalFloorAtlasPages={page_count};\n" +
    "}\n", encoding="utf-8")
print(f"Original Map: {len(instances)} authored tilemap strips; {len(tile_keys)} original source tiles across all seven ground sheets; "
      f"{page_count} authored atlases plus {len(SOURCE_GROUND)} original map-level ground bases; "
      f"packed layout {destination.stat().st_size} bytes")
