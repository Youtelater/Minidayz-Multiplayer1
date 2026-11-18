#!/usr/bin/env python3
"""Inspect exact original ground tilemaps and inventory sprite frames."""
import json,re
from pathlib import Path
import struct

def dimensions(p):
    data=p.read_bytes()[:24]
    if data[:8]!=b'\x89PNG\r\n\x1a\n': raise ValueError(p)
    return struct.unpack('>II',data[16:24])
root=Path(__file__).resolve().parents[2]
p=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")))["project"]
words=("ground_lvl1_tilemap","ground_lvl2_tilemap","ground_enviroment_tilemap",
       "gui_inventory-sheet","gui_inventory_card-sheet","gui_inv_card_",
       "gui_slot-sheet","gui_slot_bg-sheet","gui_item_")
for i,obj in enumerate(p[3]):
    s=json.dumps(obj,separators=(",",":"))
    if any(w in s for w in words):
        print("OBJECT",i,"TYPE",obj[1],"HEAD",str(obj[:8])[:400])
        for anim in obj[7] if len(obj)>7 and isinstance(obj[7],list) else []:
            frames=anim[7] if len(anim)>7 and isinstance(anim[7],list) else []
            print("  ANIMATION",anim[0],"frames",len(frames))
            for fr in frames[:2]:
                if isinstance(fr,list):
                    print("    FRAME",str(fr[:10])[:260])
for path in sorted((root/"assets/gfx").glob("ground_*tilemap.png")):
    print("GROUND_IMAGE",path.name,"SIZE",dimensions(path))
for path in sorted((root/"assets/gfx").glob("gui_inventory*png"))[:20]:
    print("UI_IMAGE",path.name,"SIZE",dimensions(path))
for path in sorted((root/"assets/gfx").glob("gui_slot*png"))[:20]:
    print("UI_IMAGE",path.name,"SIZE",dimensions(path))
for l in p[5]:
    if l[0]!="Map":continue
    for y,layer in enumerate(l[6]):
        if any(q in layer[0].lower() for q in ("ground","floor","terrain")):
            print("GROUND_LAYER",y,layer[0],"HEAD",str(layer[:14])[:250])
            for inst in layer[14][:2] if len(layer)>14 and isinstance(layer[14],list) else []:
                print("  INSTANCE",str(inst)[:1600])
