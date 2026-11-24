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
        'begin_source_generation(world_seed_, "Map", world_level_, 0, legacy_world_generation_)' in game and
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
