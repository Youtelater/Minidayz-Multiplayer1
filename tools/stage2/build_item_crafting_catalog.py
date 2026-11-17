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
SECONDARY = ROOT / "include/minidayz/generated/item_secondary_crafting_catalog.hpp"
NEW_ITEMS = ROOT / "l_eng_new.xml"
ITEM_TABLE = ROOT / "mp_itemtable.js"

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
    for path in (ITEMS, UI, PROJECT, SECONDARY, NEW_ITEMS, ITEM_TABLE):
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
        "Crafted Arrow": (5857097494378506, "Craft_21"),
        "Crafted Arrow Alternate": (6171369527712243, "Craft_22"),
        "Laser AK Grip": (8016043411981855, "Craft_ak"),
        "Laser Rail Grip": (7958912387827208, "Craft_rail"),
        "Molotov Cocktail": (554787549161111, "Craft_molotov_flare"),
        "Molotov Cocktail Alternate": (832456160298927, "Craft_molotov_whiskey"),
        "Canvas": (596327516047430, "Craft_tent"),
        "Makeshift Suppressor": (9837716596519874, "Craft_xiaoyinqi"),
        "Simple Fishing Rod": (642735773637272, "Craft_fishing_rod"),
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
        world_spawn_actions = []
        world_condition_actions = []
        for nested in child_events([event]):
            for action in nested[6] if isinstance(nested[6], list) else []:
                if not isinstance(action, list) or len(action) < 6:
                    continue
                if action[0:2] == [-1, 94]:
                    world_spawn_actions.append(action)
                if action[0:2] == [837, 88]:
                    world_condition_actions.append(action)
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
        if result_name == "Simple Fishing Rod":
            # Original Craft_fishing_rod creates object 837 on the
            # items_on_ground layer at Player.X, Player.Y + 10, then sets
            # its original instance variable 1 to 100.
            expected_spawn = [
                [4, 837], [5, [2, "items_on_ground"]],
                [0, [20, 181, 65, False, None]],
                [0, [4, [20, 181, 66, False, None], [0, 10]]],
            ]
            exact = [a for a in world_spawn_actions
                     if a[5] == expected_spawn]
            exact_condition = [a for a in world_condition_actions
                               if a[5] == [[10, 1], [7, [0, 100]]]]
            if output_ids or inventory_output_ids or len(exact) != 1 or len(exact_condition) != 1:
                raise SystemExit("Craft_fishing_rod original world item changed")
            return False
        if result_name in ("Campfire Kit", "Crafted Arrow", "Crafted Arrow Alternate",
                            "Laser AK Grip", "Laser Rail Grip",
                            "Molotov Cocktail", "Molotov Cocktail Alternate",
                            "Makeshift Suppressor"):
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

    # Original Craft_21 and Craft_22 both return one item 51 through
    # Check_space_inventory. Root tutorial says Wooden Sticks + any knife;
    # all three exact root item names containing "Knife" are catalysts.
    for name in ("Crafted Arrow", "Crafted Arrow Alternate"):
        assert_original_output(name, resolve("Crafted Arrow"))
    knives = ("Hunting Knife", "Army Knife", "Butcher Knife")
    source_knife_names = [name for name in item_names if "Knife" in name]
    if sorted(source_knife_names) != sorted(knives):
        raise SystemExit(f"Original knife choices changed: {source_knife_names}")
    arrow_guide = [line for line in tutorial if re.fullmatch(
        r"Wooden sticks\s*\+\s*any knife\s*=\s*arrows",
        line, re.IGNORECASE)]
    if len(arrow_guide) != 1:
        raise SystemExit("Original root arrow recipe guide changed")

    # Source-validated item_activate2 recipe, not a Craft_* function.
    # The secondary catalog generator verifies the exact Construct action.
    secondary_source = SECONDARY.read_text(encoding="utf-8")
    def secondary_id(name: str) -> int:
        match = re.search(
            rf"kSourceRagsToRope{name}\s*=\s*(\d+)\s*;", secondary_source)
        if not match:
            raise SystemExit(f"Missing verified secondary craft {name}")
        return int(match.group(1))
    if (secondary_id("InputId") != resolve("Rags") or
        secondary_id("RequiredCount") != 3 or
        secondary_id("OutputId") != resolve("Rope")):
        raise SystemExit("Verified Rags to Rope recipe changed")

    # Original root l_eng_new.xml provides these ingredients verbatim.
    # Verify that the Construct source creates the item in inventory.
    descriptions = {
        int(row.findtext("id")): (row.findtext("name") or "")
        for row in ET.parse(NEW_ITEMS).getroot().findall("text")
    }
    def assert_original_five_second_wait(result_name):
        uid, _ = source_event_uids[result_name]
        event = source_events[uid]
        actions = [action for nested in child_events([event])
                   for action in nested[6]
                   if isinstance(action, list) and len(action) > 5]
        waits = [a for a in actions
                 if a[0:2] == [-1, 54] and a[5] == [[0, [0, 5]]]]
        cooldowns = [a for a in actions
                     if a[0:2] == [189, 69] and len(a[5]) > 1
                     and a[5][0] == [1, [2, "Wpn_cooldown"]]
                     and a[5][1] == [13, [7, [0, 5]]]]
        if len(waits) != 1 or len(cooldowns) != 1:
            raise SystemExit(f"Original delayed craft changed: {result_name}")

    extra_recipes = (
        ("Laser AK Grip", (("Laser Sight", 1), ("AK Grip", 1)),
         "Crafting Reipe: Laser Sight + AK Grip", 0),
        ("Laser Rail Grip", (("Laser Sight", 1), ("Rail Grip", 1)),
         "Crafting Reipe: Laser Sight + Rail Grip", 0),
        ("Molotov Cocktail", (("Whiskey", 1), ("Flare", 1)),
         "Crafting Recipe: Whiskey + Flare", 0),
        ("Canvas", (("Burlap Sack", 2),),
         "Crafting Recipe: 2 Burlap Sacks", 5),
        ("Makeshift Suppressor", (("Water Bottle", 1), ("Duct Tape", 1)),
         "Crafting Recipe: Water Bottle + Duct Tape", 5),
    )
    for label, _, phrase, delay in extra_recipes:
        if descriptions.get(resolve(label), "").count(phrase) != 1:
            raise SystemExit(f"Source crafting guide changed: {label}")
        if delay == 5:
            assert_original_five_second_wait(label)
    assert_original_output("Molotov Cocktail Alternate", resolve("Molotov Cocktail"))

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
        rows.append((result_id, result_name, parts, source_text[0], bool(inventory_result), False, False, 0, 0))
    for knife in knives:
        # Exact source check gives one arrow to inventory, no Spawn_drop.
        # The original knife is a crafting tool, not a consumed ingredient.
        rows.append((resolve("Crafted Arrow"), f"Crafted Arrow / {knife}",
                     [(resolve("Wooden Sticks"), 1), (resolve(knife), 1)],
                     arrow_guide[0], True, True, False, 0, 0))

    # The selected stack, not the sum of separate Rags stacks, must
    # contain at least three. The 3DS panel selects an eligible stack.
    rows.append((resolve("Rope"), "Rope / Rags x3",
                 [(resolve("Rags"), 3), (0, 0)],
                 "item_activate2: selected Rags stack >= 3 -> Rope",
                 True, False, True, 0, 0))

    # The original table explicitly maps item ID 87 to Construct object t837.
    # Its original item descriptions explicitly list the two ingredients.
    table = ITEM_TABLE.read_text(encoding="utf-8-sig")
    source_object = re.search(r'(?:^|[,{])87:"([^"]+)"', table)
    if (source_object is None or
        source_object.group(1).encode("ascii").decode("unicode_escape") != "t837"):
        raise SystemExit("Original fishing rod item/object mapping changed")
    if ("Simple Fishing Rod" not in descriptions[resolve("Ashwood Stick")] or
        "Simple Fishing Rod" not in descriptions[resolve("Rope")]):
        raise SystemExit("Original fishing rod ingredients changed")
    assert_original_output("Simple Fishing Rod", resolve("Simple Fishing Rod"))

    for result_name, ingredients, guide, delay in extra_recipes:
        result_id = resolve(result_name)
        result_in_inventory = bool(assert_original_output(result_name, result_id))
        parts = [(resolve(label), amount) for label, amount in ingredients]
        if len(parts) == 1: parts.append((0, 0))
        if result_id in (parts[0][0], parts[1][0]):
            raise SystemExit("Self-consuming source craft")
        rows.append((result_id, result_name, parts, guide, result_in_inventory, False, False, delay, 0))

    rows.append((resolve("Simple Fishing Rod"), "Simple Fishing Rod",
                 [(resolve("Ashwood Stick"), 1), (resolve("Rope"), 1)],
                 "Original Craft_fishing_rod: Ashwood Stick + Rope",
                 False, False, False, 0, 837))

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
        "    bool preserve_second_ingredient;",
        "    bool selected_stack_only;",
        "    float delay_seconds;",
        "    std::uint16_t source_ground_object;",
        "};",
        f"inline constexpr std::size_t kSourceCraftRecipeCount = {len(rows)};",
        "inline constexpr SourceCraftRecipe kSourceCraftRecipes[] = {",
    ]
    for result_id, name, parts, guide, inventory_result, preserve_second, selected_stack_only, delay_seconds, source_ground_object in rows:
        ingredients = ", ".join(f"{{{id}, {count}}}" for id, count in parts)
        lines.append(f"    {{{result_id}, 1, {json.dumps(name)}, "
                     f"{json.dumps(guide)}, {{{{{ingredients}}}}}, "
                     f"{str(inventory_result).lower()}, {str(preserve_second).lower()}, "
                     f"{str(selected_stack_only).lower()}, {delay_seconds}.0f, {source_ground_object}}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} recipes with validated original ground/inventory outputs")


if __name__ == "__main__":
    main()
