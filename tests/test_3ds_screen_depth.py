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
    ("original road over floor","C2D_SpriteSetDepth(&sprite,0.10f)"),
    ("floor title background","0.0f, 0.0f, 400.0f, 240.0f, 0.05f"),
    ("inventory under HUD","0.0f, 0.0f, 320.0f, 240.0f, 0.05f"),
    ("player over ground at source size","p.x,p.y,1.0f,1.0f,0.57f"),
    ("source layer grouping before draw","source_sort_visible_layer_order("),
    ("source within-layer depth","source_world_layer_depth("),
    ("source order group update","if(visible_slot>=layer_end)"),
    ("launcher input neutral gate","boot_input_gate_.update(dt,menu_controls_held)"),
):
    source=engine if label in (
        "menu text foreground","bitmap font foreground","menu wood foreground",
        "original floor background","original road over floor") else main
    assert code in source, f"3DS screen depth/input regression: {label}"
# Opaque fallback panels must sit BEHIND the .05 menu-city art;
# otherwise the menu background vanishes as soon as the depth order is fixed.
assert "C2D_Color32(22,26,24,255),0.01f" in main
assert "C2D_Color32(24, 34, 29, 255),0.01f" in main
assert "float depth=0.5f" in (ROOT/"include/minidayz/engine.hpp").read_text(encoding="utf-8")
assert "kSkinPrevious{18, 145, 154, 191}" in menu
assert "kNewGameButton=kMenuRows[1]" in menu
print("PASS: emulator-tested foreground/background depths and startup controls")
