#!/usr/bin/env python3
"""Export the original Construct Map's water, sand, forest, scenery tilemaps.

These are separate from the seven 'ground_lvl' images: without them the
terrain renders only the grass backdrop, missing beaches, shorelines and
authored forest shapes. All placements and RLE cells originate in the ROOT
Construct project and original assets/gfx PNG files (never images/).
"""
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
GFX = ROOT / "assets/gfx"
DEST = ROOT / "romfs/gfx"
TILEMAPS = {
    208: "tilemap_water.png",
    211: "tilemap_forest.png",
    418: "tilemap_sand.png",
    230: "ground_enviroment_tilemap.png",
}
MASK = 0x1fffffff


def expand(payload, size):
    result = []
    for part in str(payload).split(","):
        match = re.fullmatch(r"(\d+)x(-?\d+)|(-?\d+)", part)
        if not match:
            raise ValueError(f"Malformed original RLE {part!r}")
        length = int(match.group(1) or 1)
        value = int(match.group(2) if match.group(2) is not None else
                    match.group(3))
        if length < 1 or len(result) + length > size:
            raise ValueError("Original tilemap exceeds its bounds")
        result.extend([value] * length)
    if len(result) != size:
        raise ValueError("Original tilemap cell count mismatch")
    return result


project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
    SOURCE.read_text(encoding="utf-8-sig")))["project"]
layout = next((x for x in project[5] if x[0] == "Map"), None)
if layout is None:
    raise SystemExit("Original Construct Map missing")
DEST.mkdir(parents=True, exist_ok=True)

instances = []
palette = {key: set() for key in TILEMAPS}
tile_sizes = {}
for layer_index, layer in enumerate(layout[6]):
    for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
        if not isinstance(inst,list) or len(inst)<6 or inst[1] not in TILEMAPS:
            continue
        object_id = inst[1]
        t, attrs = inst[0], inst[5]
        if len(t)<14 or not isinstance(t[13],list) or len(t[13])<3:
            raise SystemExit(f"Missing source tilemap cells for {object_id}")
        if not isinstance(attrs,list) or len(attrs)<3:
            raise SystemExit(f"Missing source cell dimensions for {object_id}")
        tw,th=int(attrs[1]),int(attrs[2])
        if tw<=0 or th<=0 or tw!=th:
            raise SystemExit(f"Invalid original source cell size for {object_id}")
        if object_id in tile_sizes and tile_sizes[object_id]!=tw:
            raise SystemExit(f"Inconsistent tile size for source object {object_id}")
        tile_sizes[object_id]=tw
        cols,rows=int(t[13][0]),int(t[13][1])
        if cols<=0 or rows<=0 or cols*rows>2000000:
            raise SystemExit("Invalid source tilemap dimensions")
        cells=expand(t[13][2],cols*rows)
        palette[object_id].update(c & MASK for c in cells if c!=-1)
        instances.append((object_id,layer_index,
                          float(t[0]),float(t[1]),cols,rows,cells))

if not instances or set(TILEMAPS)-{x[0] for x in instances}:
    raise SystemExit("Original water/sand/forest/scenery placements missing")
with tempfile.TemporaryDirectory(prefix="mdz_source_terrain_") as scratch:
    work=Path(scratch)
    mapping={}
    for object_id,filename in TILEMAPS.items():
        obj=project[3][object_id]
        if obj[1]!=16 or Path(obj[6][0]).name!=filename:
            raise SystemExit(f"Original terrain object changed: {object_id}")
        tile_size=tile_sizes[object_id]
        with Image.open(GFX/filename) as im:
            art=im.convert("RGBA")
            # Construct's water and sand sheets are 180x301 PNGs: the
            # extra bottommost pixel is transparent export padding, not
            # another tile row. Keep their exact 60x60 source tile crops.
            if art.width%tile_size or art.height%tile_size>1:
                raise SystemExit(f"Bad terrain sheet dimensions: {filename}")
            columns=art.width//tile_size
            count=columns*(art.height//tile_size)
            ordered=sorted(palette[object_id])
            if len(ordered)>64 or not ordered:
                raise SystemExit(f"Unexpected source terrain tile count: {object_id}")
            directory=work/str(object_id)
            directory.mkdir()
            names=[]
            for frame,tile_id in enumerate(ordered):
                if tile_id>=count:
                    raise SystemExit(f"Source tile {object_id}:{tile_id} beyond sheet")
                x=(tile_id%columns)*tile_size
                y=(tile_id//columns)*tile_size
                name=f"tile_{frame:03d}.png"
                art.crop((x,y,x+tile_size,y+tile_size)).save(directory/name)
                names.append(name)
                mapping[object_id,tile_id]=frame
            (directory/"sheet.t3s").write_text(
                "--atlas -f rgba -z auto"+chr(10)+
                chr(10).join(names)+chr(10),encoding="utf-8")
            atlas=DEST/f"original_map_detail_{object_id}.t3x"
            subprocess.run(["tex3ds","-i",str(directory/"sheet.t3s"),
                            "-o",str(atlas)],cwd=directory,check=True)
            if not atlas.is_file() or atlas.stat().st_size<=32:
                raise SystemExit(f"Original terrain atlas missing: {object_id}")

# MDT1 header: magic + instance count; record: object, layer (2x u16),
# x, y (float32), tile size, columns, rows (3x u32), then 32-bit
# sprite descriptors; 0xffffffff is a missing original tile.
path=DEST/"original_map_details_layout.bin"
with path.open("wb") as output:
    output.write(struct.pack("<4sI",b"MDT1",len(instances)))
    for object_id,layer,x,y,cols,rows,cells in instances:
        output.write(struct.pack("<HHffIII",object_id,layer,x,y,
                                 tile_sizes[object_id],cols,rows))
        frames=[0xffffffff if cell==-1 else
                mapping[object_id,cell & MASK] for cell in cells]
        output.write(struct.pack("<"+"I"*len(frames),*frames))
print(f"Exported {len(instances)} original environment tilemaps, "
      f"{sum(len(i[-1]) for i in instances)} cells, "
      f"{sum(len(p) for p in palette.values())} source frames "
      "(water, sand, forest, ground environment)")
