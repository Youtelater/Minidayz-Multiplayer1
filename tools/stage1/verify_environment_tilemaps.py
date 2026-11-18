#!/usr/bin/env python3
"""Verify all exported original Map detail tiles against Construct source.

Tests exact object IDs, map-layer identities, tile dimensions, positions,
and each occupied/empty source cell, not merely the presence of atlases.
"""
import json
import re
import struct
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
source=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
    (ROOT/"data/stage1/construct_project_parsed.json").read_text(
    encoding="utf-8-sig")))["project"]
layout=next(x for x in source[5] if x[0]=="Map")
objects={208,211,418,230}
instances=[]
palette={obj:set() for obj in objects}
for layer_index,layer in enumerate(layout[6]):
    for ins in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
        if not isinstance(ins,list) or len(ins)<6 or ins[1] not in objects:
            continue
        obj=ins[1]
        raw=ins[0][13]
        expected=int(raw[0])*int(raw[1])
        expanded=[]
        for term in str(raw[2]).split(","):
            matched=re.fullmatch(r"(\d+)x(-?\d+)|(-?\d+)",term)
            assert matched is not None
            value=int(matched.group(2) if matched.group(2) is not None
                      else matched.group(3))
            expanded.extend([value]*int(matched.group(1) or 1))
        assert len(expanded)==expected
        palette[obj].update(c&0x1fffffff for c in expanded if c!=-1)
        instances.append((obj,layer_index,float(ins[0][0]),float(ins[0][1]),
                          int(ins[5][1]),int(raw[0]),int(raw[1]),expanded))
# Runtime-painted Construct Tilemaps 208 and 230 package every original
# source frame (including IDs absent from preauthored Map RLE).
source_sizes={obj:next(row[4] for row in instances if row[0]==obj)
              for obj in objects}
mapping={}
for obj in objects:
    filename=Path(source[3][obj][6][0]).name
    with Image.open(ROOT/"assets/gfx"/filename) as image:
        cell=source_sizes[obj]
        ordered=list(range((image.width//cell)*(image.height//cell)))
    assert palette[obj] <= set(ordered)
    mapping[obj]={tile:i for i,tile in enumerate(ordered)}
assert len(instances)>10 and {x[0] for x in instances}==objects
file=ROOT/"romfs/gfx/original_map_details_layout.bin"
with file.open("rb") as data:
    header=data.read(8)
    magic,count=struct.unpack("<4sI",header)
    assert magic==b"MDT1" and count==len(instances)
    total=0
    for obj,layer,x,y,size,cols,rows,cells in instances:
        record=data.read(24)
        assert len(record)==24
        stored_obj,stored_layer,xx,yy,ts,cc,rr=struct.unpack("<HHffIII",record)
        assert (stored_obj,stored_layer,ts,cc,rr)==(obj,layer,size,cols,rows)
        assert abs(xx-x)<.05 and abs(yy-y)<.05
        buffer=data.read(4*len(cells))
        assert len(buffer)==4*len(cells)
        unpacked=struct.unpack("<"+"I"*len(cells),buffer)
        for source_cell,native in zip(cells,unpacked):
            expected_value=(0xffffffff if source_cell==-1
                            else (source_cell&0xe0000000)|mapping[obj][source_cell&0x1fffffff])
            assert native==expected_value,(obj,source_cell,native,expected_value)
        total+=len(cells)
    assert not data.read(1)
for obj in objects:
    atlas=ROOT/"romfs/gfx"/f"original_map_detail_{obj}.t3x"
    assert atlas.is_file() and atlas.stat().st_size>32
print(f"Verified {len(instances)} original water/forest/sand/environment tilemaps, "
      f"{total} exact cells and {len(objects)} source atlas assets")
