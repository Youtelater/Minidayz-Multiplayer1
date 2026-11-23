#!/usr/bin/env python3
"""Extract every playable source player skin from original Construct frames.

No fixed guessed object IDs: inspect the source PNG path of every animation
frame and require original idle/run directional poses. All skin frames are
already converted by build_sprite_catalog.py from assets/gfx.
"""
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
PROJECT=ROOT/"data/stage1/construct_project_parsed.json"
OUT=ROOT/"include/minidayz/generated/player_skin_source.hpp"
DEFAULT=679
DIRECTIONS=("down","up","left","right")

def main():
    source=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
                  PROJECT.read_text(encoding="utf-8-sig"))
    objects=json.loads(source)["project"][3]
    skins=[]
    for index,obj in enumerate(objects):
        animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
        images=set()
        named={}
        for animation in animations:
            if not isinstance(animation,list) or len(animation)<=7:
                continue
            name=str(animation[0]).lower()
            frames=animation[7] if isinstance(animation[7],list) else []
            for frame in frames:
                if isinstance(frame,list) and frame and isinstance(frame[0],str):
                    basename=Path(frame[0].replace("\\","/")).name.lower()
                    if basename.startswith("player_skin_") and "-sheet" in basename:
                        images.add(basename.split("-sheet")[0])
            if frames:
                named[name]=len(frames)
        # The original biker (Sprite744) mixes player_skin_biker and
        # player_skin_def_3 frames inside its AUTHORED run/axe animations.
        # A one-PNG-family filter wrongly deleted this entire valid skin.
        # Identify a skin by its actual idle_down frame, while leaving
        # every animation's mixed source PNG references unchanged.
        idle=next((a for a in animations if isinstance(a,list) and
                   len(a)>7 and str(a[0]).lower()=="idle_down" and
                   isinstance(a[7],list) and a[7]),None)
        if idle is None or not isinstance(idle[7][0],list) or not idle[7][0]:
            continue
        first_path=idle[7][0][0]
        if not isinstance(first_path,str):
            continue
        label=Path(first_path.replace("\\","/")).name.lower().split("-sheet")[0]
        if not label.startswith("player_skin_") or label=="player_skin_dead":
            continue
        if not all(named.get(f"idle_{d}",0)>0 and
                   named.get(f"run_{d}",0)>0 for d in DIRECTIONS):
            continue
        skins.append((index,label.removeprefix("player_skin_")))
    # Retain existing 3DS selection indices across old menu.cfg saves; append
    # the recovered original biker rather than reassigning every saved skin.
    skins.sort(key=lambda row:(row[0]!=DEFAULT,row[0]==744,row[1],row[0]))
    if not skins or skins[0][0]!=DEFAULT:
        raise SystemExit("Default original player skin object 679 is missing its animations")
    if len(skins)!=21 or not any(row[0]==744 for row in skins):
        raise SystemExit(
            f"Original 21 skin objects (including biker Sprite744) not all recovered: {skins}")
    lines=["#pragma once","#include <cstddef>","#include <cstdint>",
           "namespace minidayz::stage1 {",
           "struct OriginalPlayerSkin { std::uint16_t object; const char* name; };",
           f"inline constexpr std::size_t kOriginalPlayerSkinCount={len(skins)};",
           "inline constexpr OriginalPlayerSkin kOriginalPlayerSkins[]={"]
    lines.extend("  {"+str(obj)+","+json.dumps(label)+"}," for obj,label in skins)
    lines.extend(["};","} // namespace minidayz::stage1",""])
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text("\n".join(lines),encoding="utf-8")
    print(f"Verified {len(skins)} original playable player skins with "
          "idle/run animations in all four directions")
    for object_id,name in skins:
        print(f"Skin {object_id}: {name}")

if __name__=="__main__":
    main()
