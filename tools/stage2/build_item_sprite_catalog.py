#!/usr/bin/env python3
"""Map original Mini DAYZ item IDs to their original Construct sprite objects.

The mapping is taken from the supplied game's MDZItemTable (mp_itemtable.js),
then resolved against the original Construct object table. No filename guesses
or substitute artwork are used.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ITEM_TABLE = ROOT / "mp_itemtable.js"
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/item_sprite_catalog.hpp"


def main() -> None:
    if not ITEM_TABLE.is_file() or not PROJECT.is_file():
        raise SystemExit("Missing original mp_itemtable.js or parsed Construct project")

    source = ITEM_TABLE.read_text(encoding="utf-8-sig")
    # The original export escaped even the JavaScript property name as Unicode.
    source = re.sub(r"\\u([0-9a-fA-F]{4})",
                    lambda match: chr(int(match.group(1), 16)), source)
    marker = source.find("MDZItemTable")
    start = source.find("{", marker)
    end = source.find("}", start)
    if marker < 0 or start < 0 or end < 0:
        raise SystemExit("Could not locate original MDZItemTable")

    entries: dict[int, str] = {}
    for match in re.finditer(r'(\d+)\s*:\s*("[^"]*")',
                             source[start + 1:end]):
        item_id = int(match.group(1))
        object_name = json.loads(match.group(2))
        if item_id in entries:
            raise SystemExit(f"Duplicate item sprite mapping: {item_id}")
        entries[item_id] = object_name

    # The same supplied MP source also includes two additional tables:
    # MDZPlayerDropTable (ground drop sprite for multiplayer survivors)
    # and MDZItemVariants (variant object name -> base artwork object).
    # Preserve them as explicit reference-only data, never silently alter
    # single-player loot/starting items or invent ID mappings.
    def read_mp_object_table(table_name: str) -> dict[int, str]:
        matched = re.search(
            r'window\["' + re.escape(table_name) + r'"\]\s*=\s*\{([^}]*)\}',
            source)
        if matched is None:
            raise SystemExit(f"Missing multiplayer object table {table_name}")
        parsed: dict[int, str] = {}
        for entry in re.finditer(r'(\d+)\s*:\s*("[^"]*")', matched.group(1)):
            key = int(entry.group(1))
            if key in parsed:
                raise SystemExit(f"Duplicate {table_name} ID: {key}")
            parsed[key] = json.loads(entry.group(2))
        if not parsed:
            raise SystemExit(f"Empty {table_name}")
        return parsed

    drop_entries = read_mp_object_table("MDZPlayerDropTable")
    variant_match = re.search(r'window\["MDZItemVariants"\]\s*=\s*\{([^}]*)\}', source)
    if variant_match is None:
        raise SystemExit("Missing MDZItemVariants mapping")
    variant_entries: dict[str, str] = {}
    for match in re.finditer(r'("[^"]+")\s*:\s*("[^"]+")',variant_match.group(1)):
        variant_entries[json.loads(match.group(1))] = json.loads(match.group(2))

    project_text = PROJECT.read_text(encoding="utf-8-sig")
    project_text = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", project_text)
    project = json.loads(project_text)
    objects = project["project"][3]
    object_indices: dict[str, int] = {}
    for index, obj in enumerate(objects):
        if isinstance(obj, list) and obj and isinstance(obj[0], str):
            object_indices.setdefault(obj[0], index)

    rows: list[tuple[int, int]] = []
    missing: list[tuple[int, str]] = []
    for item_id, object_name in sorted(entries.items()):
        object_index = object_indices.get(object_name)
        if object_index is None:
            missing.append((item_id, object_name))
            continue
        obj = objects[object_index]
        animations = obj[7] if len(obj) > 7 and isinstance(obj[7], list) else []
        if not any(isinstance(anim, list) and len(anim) > 7 and anim[7]
                   for anim in animations):
            missing.append((item_id, object_name))
            continue
        if object_index > 65535:
            raise SystemExit(f"Construct object index out of range: {object_index}")
        rows.append((item_id, object_index))

    drop_rows: list[tuple[int, int]] = []
    unresolved_drops: list[tuple[int,str]] = []
    for identifier, name in sorted(drop_entries.items()):
        index = object_indices.get(name)
        if index is None:
            unresolved_drops.append((identifier,name))
        else:
            drop_rows.append((identifier,index))
    variant_rows: list[tuple[int,int]] = []
    unresolved_variants: list[tuple[str,str]] = []
    for name, base in sorted(variant_entries.items()):
        object_index=object_indices.get(name)
        base_index=object_indices.get(base)
        if object_index is None or base_index is None:
            unresolved_variants.append((name,base))
        else:
            variant_rows.append((object_index,base_index))
    if not drop_rows:
        raise SystemExit("None of the supplied multiplayer player-drop sprites resolve")
    if not variant_rows:
        raise SystemExit("None of the supplied multiplayer variant sprites resolve")

    if not rows:
        raise SystemExit("No original item sprites resolved; refusing to generate an empty catalog")

    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct ItemSpriteDefinition { std::uint16_t item_id; std::uint16_t object; };",
        f"inline constexpr std::size_t kItemSpriteDefinitionCount = {len(rows)};",
        "inline constexpr ItemSpriteDefinition kItemSpriteDefinitions[] = {",
    ]
    lines.extend(f"    {{{item_id}, {object_index}}}," for item_id, object_index in rows)
    lines += ["};"]
    lines += [
        "// Multiplayer add-on references only; do not change source single-player spawn.",
        f"inline constexpr std::size_t kMpPlayerDropSpriteCount = {len(drop_rows)};",
        "inline constexpr ItemSpriteDefinition kMpPlayerDropSpriteDefinitions[] = {",
    ]
    lines.extend(f"    {{{item_id}, {object_index}}}," for item_id, object_index in drop_rows)
    lines += [
        "};",
        "struct SourceSpriteVariant { std::uint16_t object, base_object; };",
        f"inline constexpr std::size_t kMpSpriteVariantCount = {len(variant_rows)};",
        "inline constexpr SourceSpriteVariant kMpSpriteVariants[] = {",
    ]
    lines.extend(f"    {{{object_index}, {base_index}}}," for object_index, base_index in variant_rows)
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} exact item-to-Construct-sprite mappings "
          f"from {len(entries)} original table entries")
    if missing:
        print(f"Unresolved original mappings: {len(missing)} (left without an icon)")
        for item_id, name in missing[:20]:
            print(f"  item {item_id}: {name}")
    print(f"Multiplayer reference: {len(drop_rows)}/{len(drop_entries)} player-drop mappings, "
          f"{len(variant_rows)}/{len(variant_entries)} sprite variant mappings")
    if unresolved_drops:
        print(f"Unresolved multiplayer player drops (not guessed): {unresolved_drops[:12]}")
    if unresolved_variants:
        print(f"Unresolved multiplayer variants (not guessed): {unresolved_variants[:12]}")


if __name__ == "__main__":
    main()
