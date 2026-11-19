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
    ("player over ground","p.x,p.y,0.55f,0.55f,0.57f"),
    ("world placements ordered forward","0.15f + 0.0125f * static_cast<float>(placement->layer)"),
    ("launcher input neutral gate","boot_input_gate_.update(dt,menu_controls_held)"),
):
    source=engine if label in (
        "menu text foreground","bitmap font foreground","menu wood foreground",
        "original floor background","original road over floor") else main
    assert code in source, f"3DS screen depth/input regression: {label}"
assert "kSkinPrevious{18, 174, 155, 207}" in menu
assert "kNewGameButton{18, 115, 302, 163}" in menu
print("PASS: emulator-tested foreground/background depths and startup controls")
