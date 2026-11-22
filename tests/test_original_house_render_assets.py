#!/usr/bin/env python3
"""Require each original 3DS house and interior sprite frame to be packaged."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
houses=(ROOT/"include/minidayz/generated/source_house_companions.hpp").read_text(encoding="utf-8")
sprites=(ROOT/"include/minidayz/generated/sprite_catalog.hpp").read_text(encoding="utf-8")
def rows(source,name):
    parts=source.split(name+"[]={",1)
    if len(parts)!=2:parts=source.split(name+"[] = {",1)
    assert len(parts)==2,name
    return [s.strip().strip(",").strip("{}").split(",")
            for s in parts[1].split("};",1)[0].splitlines()
            if s.strip().startswith("{")]
bases={(int(p[1]),0,0) for p in rows(houses,"kSourceHousePrimarySpawns")}
companions={(int(p[5]),int(p[9]),int(p[10])) for p in rows(houses,"kSourceHouseCompanionSpawns")}
assert len(bases)>=15 and len(companions)>=20,(len(bases),len(companions))
frames={}
for p in rows(sprites,"kSpriteFrames"):
    key=tuple(int(x) for x in p[:3])
    assert key not in frames,("duplicate source frame",key)
    frames[key]=p[4].strip('"')
assert len(frames)>=1000,len(frames)
wanted=bases|companions
missing=sorted(wanted-frames.keys())
assert not missing,("source building frames unavailable",missing[:30])
missing_textures=sorted({frames[key] for key in wanted
    if not (ROOT/"romfs/gfx"/(frames[key]+".t3x")).is_file()})
assert not missing_textures,("missing 3DS building texture pages",missing_textures[:30])
print("PASS authentic 3DS house art",len(bases),"house bases",len(companions),"companion frames")
