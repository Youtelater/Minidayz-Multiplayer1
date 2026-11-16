#!/usr/bin/env python3
"""Generate only crafting recipes explicitly documented in the original root UI.

These recipes come from root l_eng_ui.xml tutorial text and root
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
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
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
    # The original tutorial lists three distinct tinder alternatives. The
    # item catalog calls the source "papers" item "Newspapers".
    ("Campfire Kit", (("Woodpiles", 1), ("Rags", 1)),
     r"Wood piles\s*\+\s*rags/wooden sticks/papers\s*=\s*fireplace kit"),
    ("Campfire Kit", (("Woodpiles", 1), ("Wooden Sticks", 1)),
     r"Wood piles\s*\+\s*rags/wooden sticks/papers\s*=\s*fireplace kit"),
    ("Campfire Kit", (("Woodpiles", 1), ("Newspapers", 1)),
     r"Wood piles\s*\+\s*rags/wooden sticks/papers\s*=\s*fireplace kit"),
)


def main() -> None:
    for path in (ITEMS, UI, PROJECT):
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

    # Verify exact result creation against the original Construct functions.
    # Craft_1/4/5 call Spawn_drop; Craft_3 calls Check_space_inventory(-1,23)
    # and therefore puts the campfire kit into inventory, not onto the ground.
    text = PROJECT.read_text(encoding="utf-8-sig")
    text = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", text)
    project = json.loads(text)
    source_event_uids = {
        "Improvised Bow": (4860170170632921, "Craft_5"),
        "Improvised Sling Bag": (3394606754986424, "Craft_1"),
        "Improvised Backpack": (9521528803584300, "Craft_4"),
        "Campfire Kit": (6293809025927058, "Craft_3"),
    }

    def child_events(events):
        for event in events if isinstance(events, list) else ():
            if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
                continue
            yield event
            if len(event) > 7:
                yield from child_events(event[7])

    source_events = {}
    by_function = {}
    function_names = {pair[1] for pair in source_event_uids.values()}
    for sheet in project["project"][6]:
        if not isinstance(sheet, list) or len(sheet) < 2:
            continue
        for event in child_events(sheet[1]):
            uid = event[4]
            if uid in (pair[0] for pair in source_event_uids.values()):
                if uid in source_events:
                    raise SystemExit(f"Duplicate Construct crafting event UID {uid}")
                source_events[uid] = event
            for condition in event[5]:
                if not isinstance(condition, list) or len(condition) < 10:
                    continue
                if condition[0:2] != [292, 71]:
                    continue
                for parameter in condition[9]:
                    if (isinstance(parameter, list) and len(parameter) > 1 and
                        parameter[0] == 7 and
                        isinstance(parameter[1], list) and len(parameter[1]) > 1 and
                        parameter[1][0] == 2 and parameter[1][1] in function_names):
                        by_function.setdefault(parameter[1][1], []).append(event)

    def assert_original_output(result_name, result_id):
        uid, function = source_event_uids[result_name]
        event = source_events.get(uid)
        if event is None:
            candidates = by_function.get(function, [])
            if len(candidates) != 1:
                raise SystemExit(
                    f"Missing/duplicate source craft function {function}; "
                    f"UIDs found: {sorted(source_events)}; name matches: {len(candidates)}")
            event = candidates[0]
        if not any(condition[0:2] == [292, 71] and
                   [10, 0] in condition[9] and
                   [7, [2, function]] in condition[9]
                   for condition in event[5]):
            raise SystemExit(f"Changed Craft function signature: {function}")
        output_ids = []
        inventory_output_ids = []
        for nested in child_events([event]):
            for action in nested[6] if isinstance(nested[6], list) else []:
                if not isinstance(action, list) or len(action) < 6:
                    continue
                if (action[0:2] == [189, 69] and
                    len(action[5]) >= 2 and
                    action[5][0] == [1, [2, "Spawn_drop"]]):
                    args = action[5][1]
                    if (isinstance(args, list) and len(args) >= 2 and
                        args[0] == 13 and args[1][0] == 7 and
                        args[1][1][0] == 0):
                        output_ids.append(args[1][1][1])
                if (action[0:2] == [189, 69] and
                    len(action[5]) >= 2 and
                    action[5][0] == [1, [2, "Check_space_inventory"]]):
                    args = action[5][1]
                    if (isinstance(args, list) and len(args) >= 3 and
                        args[0] == 13 and args[1] == [7, [0, -1]] and
                        isinstance(args[2], list) and args[2][0] == 7 and
                        isinstance(args[2][1], list) and args[2][1][0] == 0):
                        inventory_output_ids.append(args[2][1][1])
        if result_name == "Campfire Kit":
            if output_ids or inventory_output_ids != [result_id]:
                raise SystemExit(
                    f"Original {function} inventory outputs {inventory_output_ids}, "
                    f"ground outputs {output_ids}; expected {result_id}")
            return True
        if inventory_output_ids:
            raise SystemExit(f"Original {function} unexpectedly writes inventory")
        if output_ids != [result_id]:
            raise SystemExit(
                f"Original {function} ground outputs {output_ids}, "
                f"not expected item {result_id}")

    rows = []
    for result_name, ingredients, pattern in RECIPES:
        source_text = [phrase for phrase in tutorial
                       if re.fullmatch(pattern, phrase, re.IGNORECASE)]
        if len(source_text) != 1:
            raise SystemExit(f"Original root recipe missing/ambiguous: {result_name}")
        result_id = resolve(result_name)
        inventory_result = assert_original_output(result_name, result_id)
        parts = [(resolve(label), count) for label, count in ingredients]
        if result_id in [id for id, _ in parts]:
            raise SystemExit(f"Self-consuming recipe: {result_name}")
        rows.append((result_id, result_name, parts, source_text[0], bool(inventory_result)))

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
        "    bool result_in_inventory;",
        "};",
        f"inline constexpr std::size_t kSourceCraftRecipeCount = {len(rows)};",
        "inline constexpr SourceCraftRecipe kSourceCraftRecipes[] = {",
    ]
    for result_id, name, parts, guide, inventory_result in rows:
        ingredients = ", ".join(f"{{{id}, {count}}}" for id, count in parts)
        lines.append(f"    {{{result_id}, 1, {json.dumps(name)}, "
                     f"{json.dumps(guide)}, {{{{{ingredients}}}}}, "
                     f"{str(inventory_result).lower()}}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} recipes with validated original ground/inventory outputs")


if __name__ == "__main__":
    main()
