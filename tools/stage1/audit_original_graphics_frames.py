#!/usr/bin/env python3
"""Audit all original MiniDAYZ graphics against Construct frame definitions.

Read-only: NEVER edit assets/gfx. Classify images conservatively:
- 'multiple_source_frames' only if Construct has >1 distinct frame rectangle.
- 'one_source_frame' does NOT prove PNG has only one image (tile atlases exist).
- 'no_frame_mapping' requires review, NOT automatic single-image treatment.
Generated .t3x packaging and correct runtime frame selection are independent.
"""
from __future__ import annotations
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
GFX = ROOT / "assets/gfx"
ROMFS = ROOT / "romfs/gfx"
PARSED = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "data/graphics_review"
PNG_REF = re.compile(r'["\']([^"\']+\.png)["\']', re.I)

def canonical(source: str) -> str:
    return Path(source.replace("\\", "/")).name.casefold()

def t3x_name(path: Path) -> Path:
    return path.with_name(path.stem.replace("-", "_").replace(".", "_") + ".t3x")

def main() -> None:
    if not PARSED.is_file():
        raise SystemExit("Missing parsed original Construct project; run stage1-assets first")
    data = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
        PARSED.read_text(encoding="utf-8-sig")))
    objects = data["project"][3]
    frames = defaultdict(list)
    source_object_ids = defaultdict(set)
    texture_refs = set()
    for object_id, obj in enumerate(objects):
        if not isinstance(obj, list):
            continue
        if len(obj)>6 and isinstance(obj[6], list):
            for value in obj[6]:
                if isinstance(value, str) and value.lower().endswith(".png"):
                    texture_refs.add(canonical(value))
        animations = obj[7] if len(obj)>7 and isinstance(obj[7], list) else []
        for animation_id, anim in enumerate(animations):
            if not isinstance(anim, list) or len(anim)<=7 or not isinstance(anim[7], list):
                continue
            for frame_id, fr in enumerate(anim[7]):
                if not isinstance(fr, list) or len(fr)<6 or not isinstance(fr[0], str):
                    continue
                name=canonical(fr[0])
                try:
                    rect=tuple(int(fr[k]) for k in (2,3,4,5))
                except (TypeError, ValueError):
                    raise SystemExit(f"Invalid source frame {name}: object {object_id}, animation {animation_id}")
                frames[name].append((object_id,animation_id,frame_id,rect))
                source_object_ids[name].add(object_id)
    # Exact source filenames in runtime calls; indirect/dynamic references
    # are not counted as proven safe.
    runtime = "\n".join(p.read_text(encoding="utf-8", errors="replace")
        for p in [*sorted((ROOT/"source").glob("*.cpp")),
                  *sorted((ROOT/"include/minidayz").rglob("*.hpp"))]
        if "/generated/" not in p.as_posix())
    full_image_draws={canonical(n) for n in
        re.findall(r'draw_original_image\s*\(\s*"([^"]+\.png)"',runtime)}
    mp_files=list(ROOT.glob("mp_*.js"))
    # Only DIRECT literal png mentions; mp_* often selects sprites by object
    # ID and those references cannot be inferred from filename substring.
    mp_direct=set()
    for script in mp_files:
        for m in PNG_REF.finditer(script.read_text(encoding="utf-8-sig", errors="replace")):
            mp_direct.add(canonical(m.group(1)))
    originals=sorted((p for p in GFX.rglob("*") if p.is_file() and
                      p.suffix.casefold()==".png"), key=lambda p:p.relative_to(GFX).as_posix())
    assert originals,"Original assets/gfx contains no PNGs"
    duplicate_names=Counter(p.name.casefold() for p in originals)
    problems=[]
    records=[]
    for p in originals:
        relative=p.relative_to(GFX)
        basename=p.name.casefold()
        dest=ROMFS/t3x_name(relative)
        with Image.open(p) as img:
            img.load()
            width,height=img.size
            transparent=(img.convert("RGBA").getchannel("A").getextrema()[0]<255)
        refs=frames.get(basename,[])
        unique_rects=sorted({f[3] for f in refs})
        invalid=[]
        clipped=[]
        for x,y,w,h in unique_rects:
            if w<=0 or h<=0 or x>=width or y>=height or x+w<=0 or y+h<=0:
                invalid.append([x,y,w,h])
            elif x<0 or y<0 or x+w>width or y+h>height:
                clipped.append([x,y,w,h])
        if invalid:problems.append({"png":str(relative),"issue":"frame_outside_image","rectangles":invalid})
        if clipped:problems.append({"png":str(relative),"issue":"source_frame_extends_beyond_image","rectangles":clipped})
        if not dest.is_file() or dest.stat().st_size==0:
            problems.append({"png":str(relative),"issue":"missing_or_empty_t3x",
                             "expected":str(dest.relative_to(ROOT))})
        if duplicate_names[basename]>1:
            problems.append({"png":str(relative),"issue":"ambiguous_duplicate_basename"})
        if len(unique_rects)>1:
            kind="multiple_source_frames"
        elif basename in texture_refs:
            kind="tilemap_or_texture_source"
        elif len(unique_rects)==1:
            kind="one_source_frame"
        else:
            kind="no_frame_mapping"
        if basename in full_image_draws and (len(unique_rects)>1 or basename in texture_refs):
            problems.append({"png":str(relative),
                             "issue":"atlas_used_as_whole_image_in_runtime",
                             "frame_rect_count":len(unique_rects)})
        records.append({
            "png":relative.as_posix(),"width":width,"height":height,
            "transparent":transparent,"source_class":kind,
            "construct_frame_references":len(refs),
            "unique_frame_rectangles":len(unique_rects),
            "source_object_ids":sorted(source_object_ids.get(basename,set())),
            "tilemap_texture_reference":basename in texture_refs,
            "whole_png_runtime_call":basename in full_image_draws,
            "mp_js_direct_png_reference":basename in mp_direct,
            "t3x":t3x_name(relative).as_posix(),
            "t3x_present":dest.is_file() and dest.stat().st_size>0,
            "rectangles":unique_rects,
        })
    source_names={p.name.casefold() for p in originals}
    missing_source=sorted((set(frames)|texture_refs)-source_names)
    for name in missing_source:
        problems.append({"png":name,"issue":"construct_source_reference_missing_png"})
    all_categories=Counter(r["source_class"] for r in records)
    report={
        "source":"Original root data.deobfuscated.js parsed by extract_construct.py; assets/gfx",
        "warning":"Packaging is NOT evidence of correct rendering; zero/one Construct frame does NOT imply a PNG has only one visual element",
        "summary":{
            "original_png_count":len(records),
            "source_class_counts":dict(sorted(all_categories.items())),
            "source_animation_frame_references":sum(len(v) for v in frames.values()),
            "original_files_with_construct_frames":sum(bool(r["construct_frame_references"]) for r in records),
            "converted_t3x_present":sum(r["t3x_present"] for r in records),
            "source_references_missing_png":len(missing_source),
            "risk_records":len(problems),
            "direct_mp_js_png_names":len(mp_direct),
        },
        "problems":problems,
        "images":records,
    }
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/"source_frame_audit.json").write_text(
        json.dumps(report,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    with (OUT/"source_frame_audit.csv").open("w",newline="",encoding="utf-8") as fh:
        fields=["png","width","height","source_class",
                "construct_frame_references","unique_frame_rectangles",
                "tilemap_texture_reference","whole_png_runtime_call",
                "mp_js_direct_png_reference","t3x","t3x_present"]
        writer=csv.DictWriter(fh,fieldnames=fields)
        writer.writeheader()
        writer.writerows({k:r[k] for k in fields} for r in records)
    high=[p for p in problems if p["issue"]=="atlas_used_as_whole_image_in_runtime"]
    lines=[
        "# MiniDAYZ original graphics audit",
        "",
        f"- Original PNGs inspected: **{len(records)}**",
        f"- PNGs with nonempty converted T3X: **{report['summary']['converted_t3x_present']}**",
        f"- Original Construct frame references: **{report['summary']['source_animation_frame_references']}**",
        f"- Missing source PNG references: **{len(missing_source)}**",
        "",
        "## Source-evidenced image categories",
        "",
        "| Classification | PNGs |","|---|---:|",
    ]
    lines += [f"| {key} | {value} |" for key,value in sorted(all_categories.items())]
    lines += ["","## Likely incorrect whole-image rendering",""]
    lines += [f"- `{p['png']}`: {p['frame_rect_count']} source rectangles" for p in high] or ["- None found by exact static-call search."]
    lines += ["","## High-priority unresolved issues",""]
    for p in problems[:60]:
        lines.append(f"- `{p['png']}`: {p['issue']}")
    lines += ["","## Limits","",
        "- Construct object IDs and sprite frame references are source-derived.",
        "- Tilemap textures and single-frame sprite sheets may contain MANY independent pictures; review these manually.",
        "- An MP script referring to object IDs may not contain a PNG filename.",
        "- T3X existence and source frame index consistency are not emulator proof.",
        "- Contact sheets in this folder enable visual review of every original PNG.",
    ]
    (OUT/"SOURCE_FRAME_AUDIT.md").write_text("\n".join(lines)+"\n",encoding="utf-8")
    print("ORIGINAL SOURCE GRAPHICS AUDIT",json.dumps(report["summary"],sort_keys=True))
    print("Source classifications:",dict(sorted(all_categories.items())))
    for p in high[:15]:
        print("SUSPECT WHOLE-ATLAS RENDER:",p["png"],"frames=",p["frame_rect_count"])
    for p in problems[:15]:
        if p["issue"]!="atlas_used_as_whole_image_in_runtime":
            print("AUDIT RISK:",p["issue"],p["png"])
    if missing_source:
        raise SystemExit(f"Construct references {len(missing_source)} absent PNG basenames")
    if sum(not r["t3x_present"] for r in records):
        raise SystemExit("Some original PNGs lack converted T3X textures")
    if any(p["issue"]=="frame_outside_image" for p in problems):
        raise SystemExit("Original Construct frame fully outside source PNG")
    print("WROTE:",OUT/"source_frame_audit.json",OUT/"source_frame_audit.csv")
if __name__=="__main__":
    main()
