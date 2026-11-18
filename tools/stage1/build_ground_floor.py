#!/usr/bin/env python3
"""Generate a 3DS floor tile from the ORIGINAL Construct Map ground tilemap.

Unlike Sprite objects (type 18), original tilemaps (type 16) have NO sprite
animation frames, so build_sprite_catalog.py cannot load their pixels.
Crop the dominant authored nonempty Map ground tile without modifying the
root assets. Source metadata and artwork are both validated.
"""
import collections
import json
import re
import subprocess
import tempfile
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"data/stage1/construct_project_parsed.json"
GFX=ROOT/"assets/gfx"
ROMFS=ROOT/"romfs/gfx"
HEADER=ROOT/"include/minidayz/generated/ground_floor_source.hpp"
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
    SOURCE.read_text(encoding="utf-8-sig")))["project"]
ground_object=482
obj=project[3][ground_object]
if obj[1]!=16 or Path(obj[6][0]).name!="ground_lvl1_tilemap.png":
    raise SystemExit("Original Map ground tilemap object changed")
layout=next((x for x in project[5] if x[0]=="Map"),None)
if layout is None: raise SystemExit("Missing original Map layout")
instances=[]
for layer in layout[6]:
    for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
        if isinstance(inst,list) and len(inst)>5 and inst[1]==ground_object:
            instances.append(inst)
if not instances: raise SystemExit("Map layout lacks original ground tilemap")
tiles=collections.Counter()
tile_width=tile_height=None
for inst in instances:
    attrs=inst[5]
    if not isinstance(attrs,list) or len(attrs)<3:
        raise SystemExit("Original Tilemap tile geometry unavailable")
    tw,th=attrs[1],attrs[2]
    if not isinstance(tw,int) or tw<=0 or tw!=th:
        raise SystemExit("Non-square source tile geometry changed")
    if tile_width is not None and (tile_width, tile_height)!=(tw,th):
        raise SystemExit("Mixed Map ground tilesizes")
    tile_width=tw
    tile_height=th
    array=inst[0][13] if len(inst[0])>13 else None
    if not isinstance(array,list) or len(array)<3:
        raise SystemExit("Original tilemap payload missing")
    for chunk in str(array[2]).split(","):
        m=re.fullmatch(r"(\d+)x(-?\d+)|(-?\d+)",chunk)
        if not m: raise SystemExit(f"Unexpected original tile RLE {chunk!r}")
        count=int(m[1] or 1)
        tile=int(m[2] if m[2] is not None else m[3])
        if tile>=0: tiles[tile]+=count
if not tiles:raise SystemExit("No authored ground tile to render")
# Preserve the original dominant Map ground tile, not a fabricated substitute.
tile_id=tiles.most_common(1)[0][0]
image=GFX/Path(obj[6][0]).name
if not image.is_file(): raise SystemExit(f"Missing {image}")
with Image.open(image) as source:
    source.load()
    cols=source.width//tile_width
    rows=source.height//tile_height
    if cols*tile_width!=source.width or rows*tile_height!=source.height:
        raise SystemExit("Original ground sheet not divisible by source tile size")
    if tile_id>=cols*rows:raise SystemExit("Authored ground tile index outside source sheet")
    x=(tile_id%cols)*tile_width
    y=(tile_id//cols)*tile_height
    source_tile=source.convert("RGBA").crop((x,y,x+tile_width,y+tile_height))
    if source_tile.getbbox() is None:raise SystemExit("Original authored ground tile is transparent")
ROMFS.mkdir(parents=True,exist_ok=True)
HEADER.parent.mkdir(parents=True,exist_ok=True)
destination=ROMFS/"original_map_ground_floor.t3x"
with tempfile.TemporaryDirectory(prefix="minidayz_ground_") as tmp:
    src=Path(tmp)/"original_floor.png"
    source_tile.save(src)
    subprocess.run(["tex3ds","-o",str(destination),str(src)],check=True)
if not destination.is_file():raise SystemExit("No generated floor atlas")
HEADER.write_text("#pragma once\n#include <cstdint>\nnamespace minidayz::stage1 {\n"
    +f"inline constexpr std::uint16_t kOriginalFloorTileObject={ground_object};\n"
    +f"inline constexpr std::uint16_t kOriginalFloorTileId={tile_id};\n"
    +f"inline constexpr float kOriginalFloorTileSize={float(tile_width):.1f}f;\n"
    +"}\n",encoding="utf-8")
print(f"ORIGINAL FLOOR object={ground_object}, source tile={tile_id}, "
      f"tile={tile_width}x{tile_height}, authored uses={tiles[tile_id]}, output={destination}")
