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

    source = ITEM_TABLE.read_text(encoding="utf-8")
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

    project = json.loads(PROJECT.read_text(encoding="utf-8"))
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
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} exact item-to-Construct-sprite mappings "
          f"from {len(entries)} original table entries")
    if missing:
        print(f"Unresolved original mappings: {len(missing)} (left without an icon)")
        for item_id, name in missing[:20]:
            print(f"  item {item_id}: {name}")


if __name__ == "__main__":
    main()
