#!/usr/bin/env python3
"""Assert the 3DS starts on an original-art MiniDAYZ menu, not in a map.

All image names are checked against the ROOT original Construct project;
Minidayz-Multiplayer-main/ and images/ duplicate sources are never used.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
original = ROOT / "data/stage1/construct_project_parsed.json"
project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
    original.read_text(encoding="utf-8-sig")))["project"]

def all_paths(node):
    if isinstance(node, str) and node.lower().endswith(".png"):
        yield Path(node.replace("\\", "/")).name
    elif isinstance(node, list):
        for value in node:
            yield from all_paths(value)
    elif isinstance(node, dict):
        for value in node.values():
            yield from all_paths(value)

authored = set(all_paths(project[3]))
original_menu = {
    "main_menu_logo-sheet0.png": "main_menu_logo_sheet0.t3x",
    "menu_btn_wood-sheet0.png": "menu_btn_wood_sheet0.t3x",
    "tiledbgmenu_city_up.png": "tiledbgmenu_city_up.t3x",
}
engine = (ROOT / "source/engine.cpp").read_text(encoding="utf-8")
main = (ROOT / "source/main.cpp").read_text(encoding="utf-8")
menu = (ROOT / "include/minidayz/boot_menu.hpp").read_text(encoding="utf-8")

for original_name, converted in original_menu.items():
    assert original_name in authored, f"Not part of ROOT original Construct: {original_name}"
    assert (ROOT / "assets/gfx" / original_name).is_file(), original_name
    atlas = ROOT / "romfs/gfx" / converted
    assert atlas.is_file() and atlas.stat().st_size > 32, converted
    if original_name != "menu_btn_wood-sheet0.png":
        assert f"romfs:/gfx/{converted}" in engine, converted
    else:
        # menu_btn_wood-sheet0.png is an atlas of MULTIPLE colored button
        # states. Drawing it whole made rainbow-striped buttons in Manic Emu.
        single = ROOT/"romfs/gfx/menu_btn_wood_single.t3x"
        assert single.is_file() and single.stat().st_size>32
        assert "romfs:/gfx/menu_btn_wood_single.t3x" in engine
        assert "romfs:/gfx/menu_btn_wood_sheet0.t3x" not in engine

for filename in ("menu_achieves_bg-sheet0.png","options_menu-sheet0.png"):
    assert filename in authored, f"Not in ROOT original Construct images: {filename}"
    assert (ROOT/"assets/gfx"/filename).is_file(), filename
    assert (ROOT/"romfs/gfx"/filename.replace("-","_").replace(".png",".t3x")).is_file(), filename
    assert f'"{filename}"' in main, f"Original {filename} artwork not used by menu"
assert "restore_continue_from_disk()" in main, "Continue must reload save from disk"
assert "generation_failed_" in main and "WORLD LOAD FAILED" in main, "Show generation failure"
assert "menu_detail_open_" in main and "menu_detail_scroll_" in main, "Original achievement/unlock descriptions must be readable"
assert "kSourceAchievementCount" in main and "kSourceUnlockCount" in main

assert "bool menu_active_{true};" in main, "Scene must start on boot menu"
constructor = main.split("explicit WorldPreviewScene(", 1)[1].split(
    "bool write_scene_save()", 1)[0]
assert "begin_world_generation(" not in constructor, "Boot must not autostart world generation"
assert "minidayz::menu::main_entry_at(" in main
assert "minidayz::menu::move_selection(" in main
assert "if (menu_active_) {" in main
assert "draw_menu_art(0," in main, "Original game logo must be on top screen"
assert "kMenuRows[i]" in main and "kMainLabels[i]" in main
assert "kContinueButton=kMenuRows[0]" in menu
assert "kNewGameButton=kMenuRows[1]" in menu
assert "kAchievementsButton=kMenuRows[2]" in menu
assert "kUnlocksButton=kMenuRows[3]" in menu
assert "kOptionsButton=kMenuRows[4]" in menu
assert "kSkinPrevious{18, 145, 154, 191}" in menu
assert "kSkinNext{166, 145, 302, 191}" in menu
assert "kCharacterButton{13, 210, 152, 239}" in menu
assert "kMultiplayerButton{167, 210, 307, 239}" in menu
assert "menu_selection_" in main and "menu_page_" in main
assert "kMainLabels[i]" in main and "move_selection(" in main
assert "stage2_audio_.set_muted" in main
assert "NOT YET PORTED TO 3DS" in main, "Incomplete features must be honest"
assert "skin_touch_action(" in main and "advance_skin(" in main
assert "menu_bitmap_font.hpp" in engine and "ui_font_glyph" in engine
assert "if(index==1 && depth<=0.5f)depth=0.75f" in engine, "Wooden button depth must be in front"
assert "C2D_AlignLeft,x,y,0.90f" in engine, "Menu text must render in foreground"
assert "0.0f,0.0f,320.0f,240.0f,0.05f" in main, "Menu background must render behind buttons"
assert "boot_input_gate_.update" in main, "Startup launcher input must not auto-start the world"
assert "if(screen==GFX_TOP)" in engine, "Bottom-screen menu must never scroll with the world"
assert "C2D_ViewTranslate(-camera_x_,-camera_y_)" in engine
assert "if(!debug_ready_||!value" not in engine, "Text cannot disappear when emulator system font is unavailable"
assert "void Renderer::draw_text" in engine and "menu::ui_font_glyph" in engine
assert "WorldPreviewScene scene(has_saved_game ? &restored : nullptr);" in main
assert "engine.set_scene(&scene)" in main
print("PASS: original root Construct artwork bundled, scene starts on menu, "
      "and both save states expose New Game / Continue correctly")
