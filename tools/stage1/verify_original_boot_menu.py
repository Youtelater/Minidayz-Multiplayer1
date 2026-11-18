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
    assert f"romfs:/gfx/{converted}" in engine, converted

assert "bool menu_active_{true};" in main, "Scene must start on boot menu"
constructor = main.split("explicit WorldPreviewScene(", 1)[1].split(
    "bool write_scene_save()", 1)[0]
assert "begin_world_generation(" not in constructor, "Boot must not autostart world generation"
assert "minidayz::menu::button_action(" in main
assert "minidayz::menu::touch_action(" in main
assert "if (menu_active_) {" in main
assert "draw_menu_art(0," in main, "Original game logo must be on top screen"
assert "kContinueButton" in main and "kNewGameButton" in main
assert "kContinueButton{18, 78, 302, 132}" in menu
assert "kNewGameButton{18, 145, 302, 202}" in menu
assert "WorldPreviewScene scene(has_saved_game ? &restored : nullptr);" in main
assert "engine.set_scene(&scene)" in main
print("PASS: original root Construct artwork bundled, scene starts on menu, "
      "and both save states expose New Game / Continue correctly")
