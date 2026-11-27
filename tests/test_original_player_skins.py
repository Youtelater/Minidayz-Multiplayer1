#!/usr/bin/env python3
"""Validate exact original playable skin frames and 3DS animation atlases."""
from __future__ import annotations
import json
import csv
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
    assert len(entries)==21 and entries[0][0]==679
    # Root Start events have 21 source skin types. The earlier 20-skin
    # extractor wrongly dropped Sprite744 (biker), because two original
    # animation frames intentionally point to player_skin_def_3 sheets.
    # Do not replace those source frame references or omit the skin.
    assert entries[-1][0]==744
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
    biker=objects[744][7]
    biker_images={Path(frame[0].replace("\\","/")).name
                  for anim in biker if isinstance(anim,list) and len(anim)>7
                  for frame in anim[7] if isinstance(frame,list) and frame
                  and isinstance(frame[0],str)}
    assert "player_skin_biker-sheet0.png" in biker_images
    assert "player_skin_def_3-sheet0.png" in biker_images
    hands_header=(ROOT/"include/minidayz/generated/player_hands_source.hpp").read_text(
        encoding="utf-8")
    assert "kOriginalPlayerHandArtCount=21;" in hands_header
    hands={int(skin):int(hand) for skin,hand in
           re.findall(r"\{(\d+),(\d+),\d+,\d+ULL\},",hands_header)}
    assert len(hands)==21
    assert hands[679]==682 and hands[744]==681 and hands[1262]==1263
    # The 21 original player skins must all have a source hand layer.
    assert set(hands)=={skin for skin,_ in entries}
    sprite_text=SPRITES.read_text(encoding="utf-8")
    animation_rows=sprite_text.split("kSpriteAnimations[] = {",1)[1].split("};",1)[0]
    exported_sprites={(int(obj),int(index)):(name,int(count),float(speed),loop=="true")
                      for obj,index,name,count,speed,loop in re.findall(
        r'\{(\d+),(\d+),"([^"]+)",(\d+),([-\d.eE+]+)f,(true|false)\}',animation_rows)}
    with (ROOT/"data/stage1/animations.csv").open(newline="",encoding="utf-8") as source:
        exported_timing={(int(row["object"]),row["name"]):
                         (int(row["frames"]),float(row["speed"]),row["loop"].lower()=="true")
                         for row in csv.DictReader(source)}
    # Construct Sprite stores animation speed at [1] and repeat count at [3].
    for obj_id in set(hands) | set(hands.values()) | {813}:
        for index,anim in enumerate(objects[obj_id][7]):
            name=anim[0]
            if obj_id!=813 and not name.startswith(("run_","idle_")):
                continue
            timing=(len(anim[7]),float(anim[1]),bool(anim[2]))
            assert exported_sprites[obj_id,index]==(name,*timing),(obj_id,name)
            assert exported_timing[obj_id,name]==timing,(obj_id,name)
    for obj_id,_ in entries:
        assert re.search(r"^\s*\{"+str(obj_id)+r",\d+,\d+,\d+,",sprite_text,re.M),obj_id
    for hands_obj in set(hands.values()):
        assert re.search(r"^\s*\{"+str(hands_obj)+r",\d+,\d+,\d+,",
                         sprite_text,re.M),hands_obj
    print(f"Verified all {len(entries)} original playable skin images and "
          "eight directional source animations per skin")

if __name__=="__main__":
    main()
