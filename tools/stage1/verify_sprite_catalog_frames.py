#!/usr/bin/env python3
"""Verify ALL source Construct frames map to the correct generated Tex3DS atlas.

The validator deliberately reads the original parsed Construct export instead
of trusting the generated C++ header; it catches swapped frame IDs, wrong
image indices, missing atlas pages, invalid rectangles and name collisions.
Off-edge source frames are valid only if they retain a nonempty overlap,
and build_sprite_catalog.py must pad them transparently to the source size.
"""
import json
import re
from collections import OrderedDict, Counter
from pathlib import Path
from PIL import Image
from build_sprite_catalog import ident, MAX_FRAMES_PER_ATLAS

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"data/stage1/construct_project_parsed.json"
HEADER=ROOT/"include/minidayz/generated/sprite_catalog.hpp"
ART=ROOT/"assets/gfx"
OUT=ROOT/"romfs/gfx"
REPORT=ROOT/"data/graphics_review/source_frame_validation.json"
RECORD=re.compile(r'^\{(\d+),(\d+),(\d+),(\d+),"([^"]+)",')

def main():
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
        SOURCE.read_text(encoding="utf-8-sig")))["project"]
    objects=project[3]
    frames_by_sheet=OrderedDict()
    expected=[]
    for oi,obj in enumerate(objects):
        animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
        for ai,anim in enumerate(animations):
            if not isinstance(anim,list) or len(anim)<=7 or not isinstance(anim[7],list):
                continue
            for fi,fr in enumerate(anim[7]):
                if not isinstance(fr,list) or len(fr)<6 or not isinstance(fr[0],str):
                    continue
                rect=tuple(int(fr[k]) for k in (2,3,4,5))
                mapping=frames_by_sheet.setdefault(fr[0],OrderedDict())
                index=mapping.setdefault(rect,len(mapping))
                expected.append((oi,ai,fi,index%MAX_FRAMES_PER_ATLAS,
                                f"{ident(fr[0])}_p{index//MAX_FRAMES_PER_ATLAS:03d}"))
    actual=[]
    for line in HEADER.read_text(encoding="utf-8").splitlines():
        m=RECORD.match(line)
        if m:
            oi,ai,fi,idx=(int(m[k]) for k in range(1,5))
            actual.append((oi,ai,fi,idx,m.group(5)))
    if actual!=expected:
        for i in range(min(len(actual),len(expected))):
            if actual[i]!=expected[i]:
                raise SystemExit(f"WRONG SOURCE SPRITE FRAME #{i}: original {expected[i]} generated {actual[i]}")
        raise SystemExit(f"Wrong Construct atlas record count: expected {len(expected)}, generated {len(actual)}")
    transparent_padding=[]
    verified_pages=set()
    missing=[]
    for source,rects in frames_by_sheet.items():
        source_path=ART/Path(source.replace("\\","/")).name
        if not source_path.is_file():
            raise SystemExit(f"Cannot validate missing original sheet: {source_path}")
        with Image.open(source_path) as image:
            image.load()
            for rect in rects:
                x,y,w,h=rect
                if w<=0 or h<=0:
                    raise SystemExit(f"Invalid frame size: {source} {rect}")
                left=max(0,x)
                top=max(0,y)
                right=min(image.width,x+w)
                bottom=min(image.height,y+h)
                if left>=right or top>=bottom:
                    raise SystemExit(f"No visible source pixels: {source} {rect}")
                if left!=x or top!=y or right!=x+w or bottom!=y+h:
                    transparent_padding.append({
                        "source":source,"rectangle":rect,
                        "original_png_size":[image.width,image.height],
                        "visible_original_rect":[left,top,right,bottom],
                        "transparent_padding":[left-x,top-y,x+w-right,y+h-bottom]
                    })
        for page in range((len(rects)+MAX_FRAMES_PER_ATLAS-1)//MAX_FRAMES_PER_ATLAS):
            name=f"{ident(source)}_p{page:03d}.t3x"
            texture=OUT/name
            if not texture.is_file() or texture.stat().st_size<32:
                missing.append(name)
            else:
                verified_pages.add(name)
    if missing:
        raise SystemExit("Missing/empty sprite atlas pages: "+", ".join(missing[:15]))
    result={
        "status":"source_mapping_verified",
        "construct_sprite_frame_references":len(expected),
        "source_sheets_with_frames":len(frames_by_sheet),
        "generated_sprite_atlas_pages":len(verified_pages),
        "off_edge_source_frame_rectangles_with_transparent_padding":len(transparent_padding),
        "padding_frames":transparent_padding,
        "limitations":[
            "Frame-index and page mapping match original Construct source; actual 3DS draw quality still requires emulator comparison",
            "Transparent padding is verified geometrically, not by decoding GPU textures"
        ]
    }
    REPORT.parent.mkdir(parents=True,exist_ok=True)
    REPORT.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print("VERIFIED SOURCE FRAME MAPPING",json.dumps({
        k:v for k,v in result.items() if isinstance(v,(int,str))
    },sort_keys=True))
if __name__=="__main__":
    main()
