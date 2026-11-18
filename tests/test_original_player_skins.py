#!/usr/bin/env python3
"""Validate exact original playable skin frames and 3DS animation atlases."""
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/"data/stage1/construct_project_parsed.json"
CATALOG=ROOT/"include/minidayz/generated/player_skin_source.hpp"
SPRITES=ROOT/"include/minidayz/generated/sprite_catalog.hpp"

def main():
    text=CATALOG.read_text(encoding="utf-8")
    entries=[(int(n),label) for n,label in
             re.findall(r'\{(\d+),"([^"]+)"\},',text)]
    count=re.search(r'kOriginalPlayerSkinCount=(\d+);',text)
    assert count is not None and int(count.group(1))==len(entries)
    assert len(entries)>=3 and entries[0][0]==679
    assert len(set(entry[0] for entry in entries))==len(entries)
    assert len(set(entry[1] for entry in entries))==len(entries)
    raw=SOURCE.read_text(encoding="utf-8-sig")
    raw=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw)
    objects=json.loads(raw)["project"][3]
    for obj_id,label in entries:
        assert 0<=obj_id<len(objects),obj_id
        anims=objects[obj_id][7]
        named={a[0]:a[7] for a in anims if isinstance(a,list) and len(a)>7}
        for movement in ("idle","run"):
            for facing in ("down","up","left","right"):
                frames=named.get(movement+"_"+facing)
                assert isinstance(frames,list) and frames,(obj_id,movement,facing)
                assert all(isinstance(f,list) and f and isinstance(f[0],str)
                           for f in frames),(obj_id,movement,facing)
                for frame in frames:
                    png=Path(frame[0].replace("\\","/")).name
                    assert (ROOT/"assets/gfx"/png).is_file(),png
    sprite_text=SPRITES.read_text(encoding="utf-8")
    for obj_id,_ in entries:
        assert re.search(r"^\s*\{"+str(obj_id)+r",\d+,\d+,\d+,",sprite_text,re.M),obj_id
    print(f"Verified all {len(entries)} original playable skin images and "
          "eight directional source animations per skin")

if __name__=="__main__":
    main()
