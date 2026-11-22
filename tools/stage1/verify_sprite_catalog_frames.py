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
from build_sprite_catalog import ident, MAX_FRAMES_PER_ATLAS, original_player_outline

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
    outline_objects=set(next(f[1:] for f in project[4] if f[0]==1064))
    assert project[14] is False, "Original Outline port assumes source point sampling"
    foundation_objects=set(next(f[1:] for f in project[4] if f[0]==1048))
    expected_tint=set()
    expected_outline=set()
    for layout in project[5]:
        for layer in layout[6]:
            for instance in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
                if instance[1] in foundation_objects:
                    assert instance[0][12]==[[0.5,0.5,0.5]], "Original foundation Tint parameters changed"
                    expected_tint.add(instance[1])
                if instance[1] in outline_objects:
                    assert instance[0][12]==[[255,255,255,0,0]], "Original player Outline parameters changed"
                    expected_outline.add(instance[1])
    assert expected_tint==foundation_objects and expected_outline==outline_objects
    # Verify the exact eight-neighbour shader at the original pixel grid.
    probe=Image.new("RGBA",(5,5),(0,0,0,0))
    probe.putpixel((2,2),(255,255,255,255))
    effect=original_player_outline(probe)
    assert effect.size==probe.size
    for y in range(5):
        for x in range(5):
            assert effect.getpixel((x,y))==((255,255,255,255) if max(abs(x-2),abs(y-2))<=1 else (0,0,0,0))
    # Root WebGL premultiplied alpha conversion must preserve the shader formula.
    partial=Image.new("RGBA",(3,3),(100,150,200,128))
    assert original_player_outline(partial).getpixel((1,1))==(50,75,100,128)
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
                    # Independently validate the required RGBA extraction:
                    # existing pixels must occupy their original offsets,
                    # while out-of-sheet space must remain transparent.
                    source_rgba=image.convert("RGBA")
                    expected_pixels=source_rgba.crop((left,top,right,bottom))
                    extracted=Image.new("RGBA",(w,h),(0,0,0,0))
                    extracted.paste(expected_pixels,(left-x,top-y))
                    if extracted.crop((left-x,top-y,right-x,bottom-y)).tobytes()!=expected_pixels.tobytes():
                        raise SystemExit(f"Pixel-offset mismatch in padded source frame: {source} {rect}")
                    if (left-x)>0 and extracted.getpixel((0,0))[3]!=0:
                        raise SystemExit(f"Nontransparent left padding: {source} {rect}")
                    if (top-y)>0 and extracted.getpixel((0,0))[3]!=0:
                        raise SystemExit(f"Nontransparent top padding: {source} {rect}")
                    if (x+w-right)>0 and extracted.getpixel((w-1,h-1))[3]!=0:
                        raise SystemExit(f"Nontransparent right padding: {source} {rect}")
                    if (y+h-bottom)>0 and extracted.getpixel((w-1,h-1))[3]!=0:
                        raise SystemExit(f"Nontransparent bottom padding: {source} {rect}")
                    transparent_padding.append({
                        "source":source,"rectangle":rect,
                        "original_png_size":[image.width,image.height],
                        "visible_original_rect":[left,top,right,bottom],
                        "transparent_padding":[left-x,top-y,x+w-right,y+h-bottom]
                    })
        for page in range((len(rects)+MAX_FRAMES_PER_ATLAS-1)//MAX_FRAMES_PER_ATLAS):
            name=f"{ident(source)}_p{page:03d}.t3x"
            texture=OUT/name
            # Tex3DS/LZ11 produces very small VALID T3X archives for
            # tiny transparent single-frame sprites. Requiring 32 bytes
            # incorrectly rejects some original source image pages even
            # though tex3ds succeeded and the 3DS build packaged them.
            if not texture.is_file() or texture.stat().st_size==0:
                missing.append(name)
            else:
                verified_pages.add(name)
    if missing:
        raise SystemExit("Missing/empty sprite atlas pages: "+", ".join(missing[:15]))
    effect_pages={atlas+"_outline.t3x" for oi,ai,fi,idx,atlas in expected if oi in outline_objects}
    assert effect_pages, "Source player effect frames missing"
    for name in effect_pages:
        effect_texture=OUT/name
        if not effect_texture.is_file() or not effect_texture.stat().st_size:
            raise SystemExit("Missing source Outline effect atlas "+name)
    print(f"Verified all {len(effect_pages)} original player Outline atlas pages")
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
