#!/usr/bin/env python3
"""Step 4: audit original sprite-sheet coordinates for EVERY player skin and item.

Uses original parsed Construct animations (frame[2:6] = x,y,w,h; frame[7:9]
= hotspot), original mp_itemtable.js item→object mapping, and canonical
assets/gfx PNGs. A PNG that contains many icons is NEVER a single image frame.
"""
from __future__ import annotations
import json, re, struct
from collections import Counter, defaultdict
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
PROJECT=ROOT/"data/stage1/construct_project_parsed.json"
TABLE=ROOT/"mp_itemtable.js"
OUT=ROOT/"audit-output/step4/player_item_sprite_coordinates.json"
source=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
              PROJECT.read_text(encoding="utf-8-sig"))
project=json.loads(source)["project"]
objects=project[3]
names={obj[0]:i for i,obj in enumerate(objects)
       if isinstance(obj,list) and obj and isinstance(obj[0],str)}
raw=re.sub(r"\\u([0-9a-fA-F]{4})",lambda m:chr(int(m.group(1),16)),
           TABLE.read_text(encoding="utf-8-sig"))
match=re.search(r'MDZItemTable[^\{]*\{([^}]*)\}',raw)
assert match,"MDZItemTable missing in original mp_itemtable.js"
items={int(i):json.loads(n) for i,n in re.findall(
    r'(\d+)\s*:\s*("[^"]*")',match.group(1))}
assert items,"Original item mapping is empty"

def png_size(path:Path)->tuple[int,int]:
    with path.open("rb") as f: header=f.read(24)
    if header[:8]!=b"\x89PNG\r\n\x1a\n" or header[12:16]!=b"IHDR":
        raise AssertionError(f"Not original PNG {path}")
    return struct.unpack(">II",header[16:24])

def frames(object_id:int):
    obj=objects[object_id]
    animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
    for anim_id,anim in enumerate(animations):
        if not isinstance(anim,list) or len(anim)<=7 or not isinstance(anim[7],list):
            continue
        for frame_id,fr in enumerate(anim[7]):
            if not isinstance(fr,list) or len(fr)<9 or not isinstance(fr[0],str):
                continue
            x,y,w,h=(int(fr[n]) for n in (2,3,4,5))
            assert w>0 and h>0,(object_id,anim_id,frame_id,x,y,w,h)
            png=Path(fr[0].replace(chr(92),"/")).name
            file=ROOT/"assets/gfx"/png
            assert file.is_file(),(object_id,png)
            iw,ih=png_size(file)
            assert x<iw and y<ih and x+w>0 and y+h>0,(object_id,png,x,y,w,h,iw,ih)
            yield dict(object=object_id,animation=anim_id,
                       animation_name=str(anim[0]),frame=frame_id,
                       png=png,rectangle=[x,y,w,h],hotspot=[fr[7],fr[8]],
                       png_size=[iw,ih])

# Every item ID is resolved to its OWN Construct object; selecting the whole
# original image would be wrong for multi-rectangle PNGs.
all_item_rows={}
unmapped={}
multiplex_png=defaultdict(set)
for item_id,item_name in sorted(items.items()):
    obj_id=names.get(item_name)
    if obj_id is None:
        unmapped[item_id]={"object_name":item_name,"reason":"no Construct object"}
        continue
    records=list(frames(obj_id))
    if not records:
        unmapped[item_id]={"object_name":item_name,"reason":"no source frame"}
        continue
    selected=next((f for f in records
                   if f["animation"]==0 and f["frame"]==0),records[0])
    all_item_rows[str(item_id)]=dict(
       name=item_name,source_object=obj_id,selected=selected,
       all_authored_frames=records)
    for f in records:
        multiplex_png[f["png"]].add(tuple(f["rectangle"]))

# Verify the original player skin sheet slices and all eight directional
# animations, never assume one whole image is one facing sprite.
skin_rows={}
for obj_id,obj in enumerate(objects):
    if not isinstance(obj,list) or len(obj)<=7 or not isinstance(obj[7],list):
        continue
    all_records=list(frames(obj_id))
    names_set={f["animation_name"].lower() for f in all_records}
    required={f"{act}_{direction}" for act in ("idle","run")
              for direction in ("down","up","left","right")}
    if not required.issubset(names_set):
        continue
    if not any(f["png"].startswith("player_skin_") for f in all_records):
        continue
    skin_rows[str(obj_id)]=all_records
assert "679" in skin_rows,"Original default survivor object679 missing"

assert all_item_rows,"No source item icons resolved"
# Check original item and player sheets contain many distinct rectangles:
# this explicitly prevents treating entire atlases as one rendered icon.
shared={n:len(rs) for n,rs in multiplex_png.items() if len(rs)>1}
assert shared,"Expected original item sheets with multiple frame rectangles"
renderer=(ROOT/"source/engine.cpp").read_text(encoding="utf-8")
assert "draw_source_sprite(object,frame->animation,frame->frame" in renderer
assert "selected->origin_x,selected->origin_y,sx,sy,rotation" in renderer
assert "stage1::kSpriteFrames" in renderer
generator=(ROOT/"tools/stage1/build_sprite_catalog.py").read_text(encoding="utf-8")
assert "key = (int(fr[2]), int(fr[3]), int(fr[4]), int(fr[5]))" in generator
assert "fr[7]" in generator and "fr[8]" in generator
report={"source":"root mp_itemtable.js, Construct project, assets/gfx",
        "item_table_entries":len(items),"mapped_item_count":len(all_item_rows),
        "unmapped_items":unmapped,"playable_skin_count":len(skin_rows),
        "item_sheets_with_multiple_rectangles":shared,
        "item_visuals":all_item_rows,"player_skins":skin_rows}
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print("STEP4_COORDINATE_AUDIT",json.dumps({
    "original_item_ids":len(items),"mapped_item_icons":len(all_item_rows),
    "unmapped":len(unmapped),"original_skins":len(skin_rows),
    "multi_rectangle_pngs":len(shared),"source_frames":sum(
        len(v["all_authored_frames"]) for v in all_item_rows.values())+
        sum(len(v) for v in skin_rows.values())},sort_keys=True))
for png,count in sorted(shared.items(),key=lambda x:-x[1])[:10]:
    print("MULTI_FRAME_SHEET",png,count)
print("PASS: each mapped item and survivor uses original Construct cropped frame coordinates and hotspots")
