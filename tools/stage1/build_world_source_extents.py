#!/usr/bin/env python3
"""Build conservative original sprite hotspot extents for world culling.

Read the unmodified Construct project, never romfs guesses. Every original
animation/frame contributes to each object's maximum hotspot-to-corner radius.
Usage: python tools/stage1/build_world_source_extents.py
       python tools/stage1/build_world_source_extents.py --check
"""
import argparse
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / "include/minidayz/generated/world_source_extents.hpp"

def generate(source):
    raw = source.read_text(encoding="utf-8-sig")
    raw = re.sub(r'(?<![A-Za-z0-9_.])-\s+(?=\d)', '-', raw)
    objects = json.loads(raw)["project"][3]
    radii = []
    for obj in objects:
        farthest = 0.0
        for anim in obj[7] if len(obj)>7 and isinstance(obj[7],list) else []:
            for frame in anim[7] if len(anim)>7 and isinstance(anim[7],list) else []:
                if not isinstance(frame,list) or len(frame)<9:
                    continue
                w,h = float(frame[4]),float(frame[5])
                ox,oy = float(frame[7]),float(frame[8])
                if not all(map(math.isfinite,(w,h,ox,oy))) or w<=0 or h<=0:
                    continue
                dx = max(abs(ox),abs(1-ox))*w
                dy = max(abs(oy),abs(1-oy))*h
                farthest = max(farthest,math.hypot(dx,dy))
        radii.append(math.ceil(farthest))
    if max(radii)>65535:
        raise ValueError("Unexpectedly large original source sprite")
    output = [
        "#pragma once",
        "#include <cstdint>",
        "#include <cstddef>",
        "// Generated from original data.deobfuscated.js Construct frame sizes and hotspots.",
        "// Maximum hotspot-to-corner radius over all original animations and frames.",
        "// Do not infer these dimensions from the converted T3X atlas page size.",
        "namespace minidayz::stage1 {",
        f"inline constexpr std::size_t kWorldSourceExtentCount={len(radii)};",
        "inline constexpr std::uint16_t kWorldSourceMaxRadius[]={",
    ]
    for i in range(0,len(radii),20):
        output.append("    "+", ".join(map(str,radii[i:i+20]))+",")
    output.extend(["};","} // namespace minidayz::stage1",""])
    return "\n".join(output)

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--check",action="store_true")
    args=p.parse_args()
    content=generate(ROOT/"data.deobfuscated.js")
    if args.check:
        if not DEST.exists() or DEST.read_text(encoding="utf-8")!=content:
            raise SystemExit("Outdated original world frame extents: regenerate")
        print("PASS: source world frame extents match all original PNG hotspot bounds")
    else:
        DEST.parent.mkdir(parents=True,exist_ok=True)
        DEST.write_text(content,encoding="utf-8")
        print(f"Created {DEST.relative_to(ROOT)}")
if __name__ == "__main__":
    main()
