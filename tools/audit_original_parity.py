#!/usr/bin/env python3
"""Audit the 3DS port against the repository-root original Construct export.

This is a reproducible source-integrity and parity-gap report, NOT a claim that
all Construct events are implemented or that hardware rendering was tested.
Run after make stage1-assets:
    python3 tools/audit_original_parity.py
"""
from __future__ import annotations
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
errors = []
warnings = []
passed = []

def require(condition, label):
    (passed if condition else errors).append(label)

def warning(label):
    warnings.append(label)

def text(path):
    p = ROOT / path
    if not p.is_file():
        errors.append("Missing " + path)
        return ""
    return p.read_text(encoding="utf-8-sig")

original = ROOT / "data.deobfuscated.js"
parsed = ROOT / "data/stage1/construct_project_parsed.json"
snapshots = sorted((ROOT / "data/stage1/source_snapshot").glob("part_*.jsonpart"))
require(original.is_file(), "Root original Construct export exists")
require(parsed.is_file(), "Construct parsed source exists")
require(bool(snapshots), "Exact original source snapshot parts exist")
if original.is_file() and snapshots:
    digest = hashlib.sha256()
    for part in snapshots:
        with part.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
    require(digest.hexdigest() == hashlib.sha256(original.read_bytes()).hexdigest(),
            "Root original and source snapshot bytes match")
if parsed.is_file() and original.is_file():
    def parse(path):
        return json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
                                 path.read_text(encoding="utf-8-sig")))
    try:
        source = parse(original)
        extracted = parse(parsed)
        require(source == extracted, "Generated Construct tree matches original")
        project = source["project"]
        require(len(project[3]) > 500 and len(project[5]) > 1,
                "Original object/layout tables are nonempty")
        require(any(layout[0] == "Map" for layout in project[5]),
                "Original Map layout is present")
        def walk_source(events):
            for event in events:
                if not isinstance(event,list) or len(event)<7: continue
                if event[0] in (0,1): yield event
                if event[0]==0 and len(event)>7:
                    yield from walk_source(event[7])
        events=list(walk_source(dict(project[6])["Game_events"]))
        by_id={e[4]:e for e in events if e[0]==0}
        require(any(e[0]==1 and e[1:4]==["respawn_radius2",0,1800]
                    for e in events),"Root original reload radius is 1800")
        def radius_guard(uid,op,radius_expr):
            return any(c[:2]==[-1,89] and len(c)>9 and c[9][2]==[8,op]
                       and c[9][3]==[7,radius_expr] for c in by_id[uid][5])
        require(radius_guard(4231114617418879,3,[4,[23,"respawn_radius2"],[0,1000]])
                and radius_guard(312510647899376,4,[23,"respawn_radius2"])
                and radius_guard(597693887903437,3,[23,"respawn_radius2"]),
                "Root startup, erase and create reload radius expressions")
        # Root category3 animation order and Choose(0,1) source guards.
        for uid,animation,child in ((1552113303134576,"red",524169683856062),
                                   (8229034889000114,"blue",8505178315734373),
                                   (7218119429510971,"yellow",1444661926066303),
                                   (471157003773759,"black",826979891303012)):
            require(any(a[:2]==[296,160] and a[5]==[[1,[2,animation]],[3,1]]
                        for a in by_id[uid][6]),"Berry animation "+animation)
            event=by_id[child]
            require(any(c[:2]==[508,71] and c[9]==[[10,3],[8,0],[7,[0,0]]]
                        for c in event[5]) and any(a[:2]==[296,72] and
                        a[5]==[[0,[19,82,[[0,0],[0,1]]]]] for a in event[6]),
                    "Original guarded berry frame choice "+animation)
        for uid,category,action in ((296267016054731,29,762479183284581),
                                     (188361261127918,37,262746706364034)):
            event=by_id[uid]
            require(any(a[1]==73 and a[3]==action and a[5]==[[4,218],[5,[2,"zed_resp"]],[7,[0,1]]]
                        for a in event[6]) and any(a[:2]==[218,88] and a[5]==[[10,0],[7,[0,category]]]
                        for a in event[6]) and any(a[:2]==[189,69] and a[5][0]==[1,[2,"Trigger_spawn"]]
                        for a in event[6]),"Original category3 loot action "+str(category))
        # Category5 source creation chains, frame actions and links.
        for uid,variant,obstacle,light in (
            (767151885662534,11,6050888076990166,0),
            (3978431721379936,12,9992093912913937,341262524995903),
            (1475362018701672,13,7389302102398237,5644133325728841),
            (3429345725247912,14,9052307172457579,5580795606807771),
            (1736343353967656,15,4199885202842253,0)):
            actions=by_id[uid][6]
            require(any(a[:2]==[439,73] and a[3]==obstacle and
                        a[5]==[[4,345],[5,[2,"zed_resp"]],[7,[0,0]]]
                        for a in actions) and any(a[:2]==[345,72] and
                        a[5]==[[0,[0,23 if variant<12 else 27]]] for a in actions),
                    "Original category5 obstacle branch "+str(variant))
            if light:
                require(any(a[:2]==[439,73] and a[3]==light and
                        a[5]==[[4,663],[5,[2,"night"]],[7,[2,"l"+str(variant-11)]]]
                        for a in actions) and any(a[:2]==[663,72] and
                        a[5]==[[0,[0,0 if variant==13 else 1]]] for a in actions),
                    "Original category5 light branch "+str(variant))
        require(any(a[:2]==[663,159] and a[5]==[[3,0]]
                    for a in by_id[3429345725247912][6]),"Original third lamp mirror")
        require('source_category46_events_ && (var1==4 || var1==6)' in (ROOT/'source/world.cpp').read_text(encoding='utf-8'),
                'Live world uses compiled original category4/6 event chains')
        timer=by_id[583575774431491][5]
        require(any(c[:2]==[-1,77] and c[9]==[[0,[0,1]]] for c in timer)
                and any(c[:2]==[-1,51] and c[9][1:]==[[8,4],[7,[0,400]]]
                        for c in timer),"Root reload one-second timer and >400 movement guard")
    except (OSError, ValueError, KeyError, IndexError, TypeError) as exc:
        errors.append("Original Construct tree invalid: " + str(exc))

for path in ("assets/gfx/main_menu_logo-sheet0.png",
             "assets/gfx/menu_btn_wood-sheet0.png",
             "assets/gfx/tiledbgmenu_city_up.png",
             "assets/gfx/ground_lvl1_tilemap.png"):
    require((ROOT / path).is_file(), "Root original image present: " + path)
for name in ("main_menu_logo_sheet0", "menu_btn_wood_sheet0",
             "tiledbgmenu_city_up", "original_map_ground_p000"):
    path = ROOT / "romfs/gfx" / (name + ".t3x")
    require(path.is_file() and path.stat().st_size > 32,
            "3DS texture packaged: " + name)

game = text("source/main.cpp")
world = text("source/world.cpp")
renderer = text("source/engine.cpp")
floor = text("tools/stage1/build_ground_floor.py")
ui_builder = text("tools/stage2/build_inventory_ui_art.py")
makefile = text("Makefile")
require("start_new_game()" in game and
        'begin_source_generation(world_seed_, "Map", world_level_, 0, legacy_world_generation_, legacy_split_roads_, legacy_crash_coordinates_, legacy_site_generation_, !legacy_start_order_, !legacy_category3_events_, !legacy_category5_events_, !legacy_category46_events_, !legacy_category7_events_)' in game and
        'world_level_ = 0;' in game and
        "advance_source_generation()" in game and
        "generation_complete()" in game,
        "New Game starts staged source Map generation before gameplay")
require("generation_phase_ = 1" in world and
        "generate_procedural_grid(generation_level_, seed_)" in world and
        "build_map_markers(generation_level_, seed_)" in world and
        "generate_source_location_proposals(generation_level_)" in world and
        "load_source_bunker_spawns(false)" in world and
        "generate_source_road_vehicles() && load_source_bridge_elements()" in world and
        "&& materialize_source_locations()" in world and
        world.index("case 3: ok") < world.index("case 4: ok") <
        world.index("case 5: ok"),
        "All five implemented world-generation stages are wired in source proposal/load order")
require("start_source_location_reload(" in game and "update_source_location_reload(" in game and
        "source_placement_active(*p)" in game and "source_location_active(patch.marker_index)" in renderer,
        "Root reload active selection controls live native world rendering and collision")
require("menu_active_ = false" in game and "npc_ready_ = npcs_.initialize(world_)" in game,
        "Successful generation enables gameplay and initializes NPCs")
require("draw_original_ground_floor(" in game and
        'romfs:/gfx/original_map_ground_layout.bin' in renderer,
        "World renderer attempts an original-source ground atlas")
require("draw_source_sprite_fitted" in game and
        "build_inventory_ui_art.py" in makefile and
        "gui_inventory-sheet0.png" in ui_builder,
        "Bottom-screen inventory uses source image mapping")
require("draw_menu_art" in game and "draw_menu_art" in renderer and
        "menu_btn_wood_single.t3x" in renderer and
        "build_menu_button_frame.py" in makefile,
        "Boot menu uses converted original image names")
require("tools/stage1/validate_stage1.py" in makefile and
        "tools/stage2/build_item_source_catalog.py" in makefile,
        "Build regenerates data from original source")

# Clearly identified equivalence gaps; these are warnings so a working 3DS
# build is not misrepresented as a fully faithful game.
require("rle_cells" in floor and "MDG3" in floor and "mapping[(object_id, raw & TILE_MASK)]" in floor,
        "Original terrain exporter keeps all source RLE cells and tilemap object identities")
require((ROOT / "romfs/gfx/original_map_ground_layout.bin").is_file(),
        "Original Map cell layout packed in RomFS")
warning("TERRAIN: level 0/1/2 artwork and all authored ground objects are packaged; full Construct procedural tile painting, later-level rules, bridges and transitions remain unported.")
if "preview_rules_.walk_speed = 72.0f" in game:
    warning("MOVEMENT: fixed preview speeds and hitbox do not establish original Construct equivalence.")
if "place_new_player_on_generated_map()" in game:
    warning("SPAWN: generated source location with collision checks is a 3DS fallback; exact original Construct spawn selection is not yet traced.")
else:
    warning("SPAWN: original Construct player spawning has not been verified.")
if "Map preview cannot enter Bridge layout" in game:
    warning("WORLD: bridge-layout travel is not yet ported.")
if "WorldPreviewScene" in game:
    warning("PORT COVERAGE: native game is still a preview scene, not a full Construct event-engine translation.")
warning("UI: original art is resized for 400x240/320x240; fidelity requires visual testing on 3DS.")
warning("COMBAT/AI: original interactions and combat require behavioral comparisons, not only source asset checks.")
warning("SAVE: loading/saving requires emulator and device round-trip tests; static audit cannot prove durability.")
warning("AUDIO: file presence alone does not prove each original event plays its correct sample.")

print("Original-to-3DS parity audit")
for label in passed:
    print("PASS:", label)
for label in warnings:
    print("GAP:", label)
for label in errors:
    print("FAIL:", label)
print(f"Result: {len(passed)} source checks passed, {len(warnings)} parity gaps, {len(errors)} failures")
sys.exit(1 if errors else 0)
