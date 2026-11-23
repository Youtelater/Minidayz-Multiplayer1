#!/usr/bin/env python3
"""Root Construct source -> renderer world-mask/preview visual role contract.

Do not infer visibility from "PNG exists" or the default animation frame.
Preserve the original assets and event conditions. This checks only the
explicitly root-proven nonvisual objects, not every game object.
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
objects=project[3]
expected={
    181:"player_collision_base-sheet0.png",
    193:"player_base-sheet0.png",
    218:"spawn_loot_point-sheet0.png",
    345:"obst_base-sheet0.png",
    346:"warmzone-sheet0.png",
    348:"big_obstacle_base-sheet0.png",
    404:"ach_city_counter-sheet0.png",
    508:"map_element-sheet0.png",
    514:"mapgen-sheet0.png",
    571:"build_ghost-sheet0.png",
    573:"build_ghost_obstacle-sheet0.png",
    857:"polygon_target_base-sheet0.png",
    984:"collision_checker_general-sheet0.png",
    347:"b_shadows-sheet0.png",
    349:"door-sheet0.png",
}
for oid,basename in expected.items():
    obj=objects[oid]
    all_png=set()
    for anim in obj[7] if len(obj)>7 and isinstance(obj[7],list) else []:
        for fr in anim[7] if len(anim)>7 and isinstance(anim[7],list) else []:
            if isinstance(fr,list) and fr and isinstance(fr[0],str):
                all_png.add(Path(fr[0].replace(chr(92),"/")).name)
    assert basename in all_png,(oid,basename,all_png)
    assert (ROOT/"assets/gfx"/basename).is_file(),basename

# The green sheets are layout staging templates at zero alpha, and never
# unconditional scenery. Runtime appearance must be tied to Building mode.
layout=next(l for l in project[5] if l[0]=="Map")
for oid in (571,573):
    instances=[inst[0] for layer in layout[6] for inst in layer[14]
        if isinstance(inst,list) and len(inst)>2 and inst[1]==oid]
    assert instances,(oid,"missing root Map template")
    assert all(i[6]==0 for i in instances),(oid,instances)
    assert all(i[1]<0 for i in instances),(oid,instances)
def walk(events,groups=()):
    for e in events if isinstance(events,list) else []:
        if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
        group=e[1][1] if isinstance(e[1],list) and len(e[1])>1 and isinstance(e[1][1],str) else ""
        path=groups+(group,) if group else groups
        yield e,path
        yield from walk(e[7],path)
for oid in (571,573):
    events=[(e,groups) for _,sheet in project[6] for e,groups in walk(sheet)
        if any(isinstance(a,list) and a and a[0]==oid
            for a in (e[6] if isinstance(e[6],list) else []))]
    assert events,(oid,"no source runtime building-mode events")
    assert all("Building mode" in groups for _,groups in events),(
        oid,[(e[4],groups) for e,groups in events if "Building mode" not in groups])
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
header=(ROOT/"include/minidayz/source_world_sprite_visibility.hpp").read_text(encoding="utf-8")
engine=(ROOT/"source/engine.cpp").read_text(encoding="utf-8")
assert "source_world_logic_marker_not_art(" in main
assert "source_world_preview_needs_live_event(" in main
assert "if(source_bed_build_.active)" in main
assert "kSourceBedGhostObject" in main
assert "source_world_preview_needs_live_event(" in header
assert "sprite_cache_failed_draws_" in engine
assert "DROPS %lu" in engine
assert "if(std::isfinite(opacity) && opacity<=0.0f)return true;" in engine
print("PASS: all 15 root-proven visual roles, original green ghost staging templates, Building mode action ancestry and nonvisual world sprite filters")

# Source category6 car trunks are Sprite241 on original Map Sunset_Dawn45:
# do not filter them away as browser-only overlay sprites. Only root-audited
# 241/45 and 1126/44 may pass the higher-layer generic world art gate.
world_view=(ROOT/"include/minidayz/terrain_visible_index.hpp").read_text(encoding="utf-8")
world_art=(ROOT/"include/minidayz/source_world_sprite_visibility.hpp").read_text(encoding="utf-8")
assert "source_world_high_layer_gameplay_art(" in world_view
assert "source_world_high_layer_gameplay_art(" in main
assert 'object==241 && std::strcmp(source_layer_name,"Sunset_Dawn")==0' in world_art
assert 'object==1126 && std::strcmp(source_layer_name,"Rain")==0' in world_art
assert "p->object!=1126" not in world_view
assert "placement->object!=1126" not in main
