#!/usr/bin/env python3
"""Verify every original source graphic is accounted for without claiming it always renders."""
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
report=json.loads((ROOT/"audit-output/graphics/full_original_visual_role_inventory.json").read_text())
images=report["images"]
assert len(images)==report["summary"]["all_original_pngs"]==1854
assert report["summary"]["construct_sprite_frames"]==14505
assert report["summary"]["missing_source_references"]==0
assert len({x["png"].casefold() for x in images})==len(images)
assert all((ROOT/"assets/gfx"/x["png"]).is_file() for x in images)
assert all(x["role_flags"] for x in images)
assert sum(report["summary"]["source_classes"].values())==len(images)
by_name={x["png"]:x for x in images}
for name,oid,role in (
 ("build_ghost-sheet0.png",571,"conditional_build_mode_preview"),
 ("build_ghost_obstacle-sheet0.png",573,"conditional_build_mode_preview"),
 ("big_obstacle_base-sheet0.png",348,"nonvisual_generic_world_pass"),
 ("obst_base-sheet0.png",345,"nonvisual_generic_world_pass"),
 ("ach_city_counter-sheet0.png",404,"nonvisual_generic_world_pass"),
):
 rec=by_name[name]
 assert oid in rec["object_ids"],(name,rec)
 assert role in rec["role_flags"],(name,rec)
assert 321 in [oid for x in images for oid in x["object_ids"]]
# These are independent statements: having original art and frames never
# licenses drawing a collision mask or off-map preview during normal play.
print("PASS: all original PNGs source-classified; 14,505 frame refs and conditional ghost/mask roles checked")
