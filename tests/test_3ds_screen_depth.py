#!/usr/bin/env python3
"""Guard screen-space layers corrected using actual Manic Emu recording.

Citro2D foreground depths are GREATER than background depths on this runtime.
A previous inverse ordering drew opaque menu/city art over the title/buttons
and full-world grass over the player, masking an otherwise passing build.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
engine=(ROOT/"source/engine.cpp").read_text(encoding="utf-8")
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
menu=(ROOT/"include/minidayz/boot_menu.hpp").read_text(encoding="utf-8")
for label,code in (
    ("menu text foreground","C2D_AlignLeft,x,y,0.90f"),
    ("bitmap font foreground","at_y+row*pixel,0.90f"),
    ("menu wood foreground","if(index==1 && depth<=0.5f)depth=0.75f"),
    ("original floor background","C2D_SpriteSetDepth(&sprite,0.04f)"),
    ("original generated ground on source bg layer","draw_original_tile(floor_sheets_[page],slot,xx,yy,tile,depth)"),
    ("floor title background","0.0f, 0.0f, 400.0f, 240.0f, 0.05f"),
    ("inventory under HUD","0.0f, 0.0f, 320.0f, 240.0f, 0.05f"),
    ("player over ground at source size","p.x,p.y,1.0f,1.0f,player_depth"),
    ("source layer grouping before draw","source_sort_visible_layer_order("),
    ("source within-layer depth","source_world_layer_depth("),
    ("source fixture dispatch after door priority","request_source_furniture();"),
    ("source pump 2-second wait","advance_source_furniture_wait(dt);"),
    ("original tilemaps use the SAME source layer depths",
     "world_source::source_world_layer_depth(map.layer,0,0)"),
    ("source order group update","if(visible_slot>=layer_end)"),
    ("launcher input neutral gate","boot_input_gate_.update(dt,menu_controls_held)"),
):
    source=engine if label in (
        "menu text foreground","bitmap font foreground","menu wood foreground",
        "original floor background","original generated ground on source bg layer",
        "original tilemaps use the SAME source layer depths") else main
    assert code in source, f"3DS screen depth/input regression: {label}"
# Family1066 is created on the exact original bg layer. Its final
# cell frames are painted once at source scale, not overlapping bands.
assert 'std::strcmp(layer.name,"bg")==0' in engine
assert "source_world_layer_depth(bg->index,0,0)" in engine
# Opaque fallback panels must sit BEHIND the .05 menu-city art;
# otherwise the menu background vanishes as soon as the depth order is fixed.
assert "C2D_Color32(22,26,24,255),0.01f" in main
assert "C2D_Color32(24, 34, 29, 255),0.01f" in main
# The 32-atlas-frame budget must never hide the original survivor and
# equipment behind an arbitrary 33rd tree/house PNG. GPU depth, not
# draw submission order, preserves the original Construct Map layers.
start=main.index("        // World sprite atlases can consume all 32 GPU cache slots")
actor=main.index("        // Keep original pixel-art proportions.",start)
player=main.index("        // Match original Construct Map layer 32 (player)",actor)
world=main.index("            // WorldGrid codes 12/13/14",start)
assert 0<=start<actor<player<world, "source actor must be rendered before world sprite cache pressure"
assert main.index("source_tree_render_depth(",world)>world, "world draw depth still controls original layering"
assert main.index("draw_equipped_layer(held_item,true",player)<world, "held source overlay must be resident"
assert main.count("        // Match original Construct Map layer 32 (player)")==1, "duplicate player draw"
# Never render a different original skin from the PlayerSystem's animation
# object. Menu selection can change independently of a restored save.
assert "const auto source_skin_object=player_.source_skin();" in main
assert "kOriginalPlayerSkins[selected_skin_].object;" not in main.split(
    "        // Match original Construct Map layer 32 (player)",1)[1].split(
    "        // Keep source rain/night atlas pages",1)[0]
# Survivor hand/weapon poses are their own ORIGINAL Construct animations.
# A posed player's skin must not inherit the unrelated run_* frame index,
# and every actual clothing overlay uses its original independent clock.
armed=main.index('char armed_animation[32]{}')
weapon_frame=main.index('const auto weapon_stance_frame =',armed)
weapon_draw=main.index('sprite_drawn=renderer.draw_source_animation(',weapon_frame)
assert armed<weapon_frame<weapon_draw
assert 'source_skin_object,armed_animation,p.animation_elapsed' in main[weapon_frame:weapon_draw]
overlay=main.index('const auto original_frame =',weapon_draw)
assert 'visual->object,name,p.animation_elapsed' in main[overlay:overlay+250]
assert '#include "minidayz/source_equipment_animation.hpp"' in main

# Original Start events pair each survivor Sprite with a separate hands
# Sprite (PlayerBase193 variable1). It belongs to source Map layer 34,
# not an invented recolor, an equipment icon, or a missing overlay.
assert '#include "minidayz/generated/player_hands_source.hpp"' in main
assert 'source_player_hands_object(source_skin_object)' in main
hands=main.index('const auto source_hands =')
hands_layer=main.index('kSourcePlayerHandsLayer',hands)
hands_draw=main.index('draw_original_hands(skin_animation)',hands)
pants=main.rfind('kSourcePlayerPantsLayer',0,hands)
assert pants>=0
clothing=main.index('kSourcePlayerClothingLayer',hands_draw)
assert pants<hands<hands_layer<hands_draw<clothing
assert 'source_hands,animation,p.animation_elapsed' in main[hands:hands_draw]
assert 'draw_original_hands(armed_hands)' in main[hands:hands_draw]

# Original Map weather/night sprite layers must composite above all gameplay
# layers. Earlier hardcoded .14/.16 rendered both overlays behind the world.
assert main.index("if (rain_timer_.active()) {",world)>world
source_rain=main.index("source_world_layer_depth(44,0,0)",world)
source_night=main.index("source_world_layer_depth(46,0,0)",world)
assert source_rain>world and source_night>source_rain, "rain44 must precede night46"
assert "camera_x, camera_y, view_w, view_h, 0.14f" not in main
assert "camera_x, camera_y, view_w, view_h, 0.16f" not in main
# Source overlay texture pages are pinned early, but their actual pixels
# are submitted at the end on correct original Map layers.
engine_header=(ROOT/"include/minidayz/engine.hpp").read_text(encoding="utf-8")
assert "pin_source_sprite_texture(" in engine_header
assert "acquire_source_sprite_page(" in engine
assert "sprite_cache_drawn_this_frame_[slot]=true" in engine
pin=main.index("renderer.pin_source_sprite_texture(")
world=main.index("            // WorldGrid codes 12/13/14",pin)
overlay=main.index("source_world_layer_depth(44,0,0)",world)
assert 0<pin<world<overlay, "reserve overlay atlas before world; draw only afterward"
assert "renderer.draw_source_sprite_fitted(" in main[world:], "source overlay must still draw"

# On-screen source census distinguishes generated-but-hidden placements
# from skipped source frames and actual RomFS/GPU loading failures.
assert "set_world_sprite_stats(" in main
assert "++world_source_requested" in main
assert "++world_source_drawn" in main
assert "++world_source_bad_frames" in main
assert "SOURCE WORLD %lu PLACED %lu DRAWN %lu BADFRAME %lu" in engine
assert "float depth=0.5f" in (ROOT/"include/minidayz/engine.hpp").read_text(encoding="utf-8")
assert "kSkinPrevious{18, 145, 154, 191}" in menu
assert "kNewGameButton=kMenuRows[1]" in menu
print("PASS: emulator-tested foreground/background depths and startup controls")
