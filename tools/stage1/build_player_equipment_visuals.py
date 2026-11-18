#!/usr/bin/env python3
"""Extract original wearable/weapon *character overlays*, not ground icons.

MDZItemTable resolves item IDs to Construct object IDs. An item is a valid
player overlay only if that original object has all four directional poses
in its sprite animations. Any missing source overlay is marked unsupported:
never draw an inventory pickup graphic on top of the survivor as a fake jacket.
"""
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"data/stage1/construct_project_parsed.json"
TABLE=ROOT/"mp_itemtable.js"
OUTPUT=ROOT/"include/minidayz/generated/player_equipment_visuals.hpp"
DIRECTIONS=("down","up","left","right")
KINDS=("idle","run")
WEAPON_KINDS=("pistol","axe","twohand","rifle","shotgun","idle","run")

def load_items():
    text=TABLE.read_text(encoding="utf-8-sig")
    text=re.sub(r"\\u([0-9a-fA-F]{4})",
                lambda match:chr(int(match.group(1),16)),text)
    start=text.find("MDZItemTable")
    left=text.find("{",start)
    right=text.find("}",left)
    if start<0 or left<0 or right<0:
        raise SystemExit("Missing original MDZItemTable")
    return [(int(m.group(1)),json.loads(m.group(2))) for m in
            re.finditer(r'(\d+)\s*:\s*("[^"]*")',text[left+1:right])]

def main():
    raw=SOURCE.read_text(encoding="utf-8-sig")
    raw=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw)
    objects=json.loads(raw)["project"][3]
    by_name={obj[0]:i for i,obj in enumerate(objects)
             if isinstance(obj,list) and isinstance(obj[0],str)}
    rows=[]
    unsupported=[]
    for item_id,name in load_items():
        index=by_name.get(name)
        if index is None:continue
        obj=objects[index]
        animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
        named={str(a[0]).lower():len(a[7]) for a in animations
               if isinstance(a,list) and len(a)>7 and isinstance(a[7],list)}
        mask=0
        for bit,prefix in enumerate(KINDS+WEAPON_KINDS):
            if all(named.get(f"{prefix}_{direction}",0)>0
                   for direction in DIRECTIONS):
                mask|=1<<bit
        if mask:
            rows.append((item_id,index,mask))
        else:
            unsupported.append((item_id,name))
    rows.sort()
    # All of these families are present in the supplied original MDZItemTable.
    # Catch an accidentally dropped equipment sprite group at build time.
    families={
        "weapons":(151,246),
        "helmets":(250,277),
        "pants":(300,307),
        "jackets":(350,368),
        "vests":(400,415),
        "backpacks":(450,462),
    }
    for label,(start,end) in families.items():
        count=sum(start<=item<=end for item,_,_ in rows)
        print(f"Original source {label} overlays: {count}")
        if count==0:
            raise SystemExit(f"Missing all original {label} directional artwork")
    lines=["#pragma once","#include <cstddef>","#include <cstdint>",
           "namespace minidayz::stage1 {",
           "struct OriginalWearableVisual { std::uint16_t item,object,animations; };",
           f"inline constexpr std::size_t kOriginalWearableVisualCount={len(rows)};",
           "inline constexpr OriginalWearableVisual kOriginalWearableVisuals[]={"]
    lines.extend(f"{{{item},{obj},{mask}}}," for item,obj,mask in rows)
    lines+=["};","} // namespace minidayz::stage1",""]
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text("\n".join(lines),encoding="utf-8")
    print(f"Original equipment character poses: {len(rows)} source-mapped items; "
          f"{len(unsupported)} source item objects lack directional animations")
    for item,obj,mask in rows[:35]:
        print(f"Original equipment overlay item {item}: object {obj}, mask {mask}")
    # Zero valid source overlays means the original game paints them through
    # a different actor object or instance-variable mechanism; the renderer
    # must not invent an overlay in that case.
if __name__=="__main__":
    main()
