#!/usr/bin/env python3
"""Generate only crafting recipes explicitly documented in the original root UI.

These three recipes come from root l_eng_ui.xml tutorial text and root
l_eng_items.xml names. No guessed quantities, alternative materials, or
recipes copied from Minidayz-Multiplayer-main are introduced.
"""
from __future__ import annotations

import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ITEMS = ROOT / "l_eng_items.xml"
UI = ROOT / "l_eng_ui.xml"
OUTPUT = ROOT / "include/minidayz/generated/item_crafting_catalog.hpp"

# The root tutorial states each recipe explicitly. The last recipe explicitly
# says 3 wooden sticks. All others are two singular item ingredients.
RECIPES = (
    ("Improvised Bow", (("Ashwood Stick", 1), ("Rope", 1)),
     r"Ashwood stick\s*\+\s*rope\s*=\s*bow"),
    ("Improvised Sling Bag", (("Rope", 1), ("Burlap Sack", 1)),
     r"Rope\s*\+\s*Burlap sack\s*=\s*improvised bag\s*\(3 slots\)"),
    ("Improvised Backpack", (("Improvised Sling Bag", 1), ("Wooden Sticks", 3)),
     r"Improvised bag\s*\+\s*3 wooden sticks\s*=\s*improvised backpack\s*\(5 slots\)"),
)


def main() -> None:
    for path in (ITEMS, UI):
        if not path.is_file():
            raise SystemExit(f"Missing root-level Mini DAYZ file: {path}")
    item_names: dict[str, list[int]] = {}
    for row in ET.parse(ITEMS).getroot().findall("item"):
        label = (row.findtext("name") or "").strip()
        if label:
            item_names.setdefault(label, []).append(int(row.findtext("id")))
    tutorial = [(row.findtext("name") or "").strip()
                for row in ET.parse(UI).getroot().findall("text")]

    def resolve(label: str) -> int:
        ids = item_names.get(label, [])
        if len(ids) != 1:
            raise SystemExit(f"Ambiguous/missing original item name {label!r}: {ids}")
        return ids[0]

    rows = []
    for result_name, ingredients, pattern in RECIPES:
        source_text = [phrase for phrase in tutorial
                       if re.fullmatch(pattern, phrase, re.IGNORECASE)]
        if len(source_text) != 1:
            raise SystemExit(f"Original root recipe missing/ambiguous: {result_name}")
        result_id = resolve(result_name)
        parts = [(resolve(label), count) for label, count in ingredients]
        if result_id in [id for id, _ in parts]:
            raise SystemExit(f"Self-consuming recipe: {result_name}")
        rows.append((result_id, result_name, parts, source_text[0]))

    lines = [
        "#pragma once",
        "#include <array>",
        "#include <cstddef>",
        "#include <cstdint>",
        "namespace minidayz::items {",
        "struct SourceCraftIngredient { std::uint16_t item_id; std::uint16_t count; };",
        "struct SourceCraftRecipe {",
        "    std::uint16_t result_id;",
        "    std::uint16_t result_count;",
        "    const char* result_name;",
        "    const char* original_guide;",
        "    std::array<SourceCraftIngredient, 2> ingredients;",
        "};",
        f"inline constexpr std::size_t kSourceCraftRecipeCount = {len(rows)};",
        "inline constexpr SourceCraftRecipe kSourceCraftRecipes[] = {",
    ]
    for result_id, name, parts, guide in rows:
        ingredients = ", ".join(f"{{{id}, {count}}}" for id, count in parts)
        lines.append(f"    {{{result_id}, 1, {json.dumps(name)}, "
                     f"{json.dumps(guide)}, {{{{{ingredients}}}}}}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} recipes from original root Mini DAYZ tutorial")


if __name__ == "__main__":
    main()
