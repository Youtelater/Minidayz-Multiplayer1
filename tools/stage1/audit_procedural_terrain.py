#!/usr/bin/env python3
"""Audit the original Construct procedural terrain-painting event tree.

The source performs SetTile actions after the original Map is loaded. Export
those actions and their ancestry verbatim so that static Map RLE is never
mistaken for complete procedural terrain. Also verify every literal tile
choice is present in the original source PNG, not only authored placements.
"""
from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data.deobfuscated.js"
DEST = ROOT / "data/stage1/original_terrain_paint_events.json"
# Four environment objects plus the seven original per-level ground objects.
# The Construct runtime can paint BOTH sets with SetTile actions; excluding
# the ground sheets silently hides the largest source-parity gap.
TERRAIN = (208, 211, 418, 230, 482, 687, 688, 776, 980, 1138, 1139)
# Original Construct Tilemap 'Set tile at' opcode, confirmed from root source.
SET_TILE = 192


def original_events(events, ancestors=()):
    for event in events:
        if not isinstance(event, list) or not event or event[0] != 0:
            continue
        yield event, ancestors
        children = event[7] if len(event) > 7 and isinstance(event[7], list) else ()
        yield from original_events(children, ancestors + (event,))


def tile_choices(tree):
    """Only literals inside Construct choose(...) are definite tile choices."""
    if not isinstance(tree, list):
        return []
    found = []
    if len(tree) >= 3 and tree[0] == 19 and tree[1] == 82:
        args = tree[2] if isinstance(tree[2], list) else []
        found.extend(int(node[1]) for node in args
                     if isinstance(node, list) and len(node) > 1
                     and node[0] == 0 and isinstance(node[1], int))
    for child in tree[1:]:
        if isinstance(child, list):
            found.extend(tile_choices(child))
    return found


def main():
    data = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
        SOURCE.read_text(encoding="utf-8-sig")))["project"]
    source_objects = data[3]
    capacity = {}
    map_layout = next(x for x in data[5] if x[0] == "Map")
    cell_sizes = {}
    for layer in map_layout[6]:
        for instance in layer[14] if len(layer) > 14 and isinstance(layer[14], list) else []:
            if not isinstance(instance, list) or len(instance) < 6 or instance[1] not in TERRAIN:
                continue
            size = int(instance[5][1])
            if size <= 0 or size != int(instance[5][2]):
                raise SystemExit(f"Invalid source terrain cell size for {instance[1]}")
            if instance[1] in cell_sizes and cell_sizes[instance[1]] != size:
                raise SystemExit(f"Conflicting source tile cell sizes for {instance[1]}")
            cell_sizes[instance[1]] = size
    # Seven level sheets share Construct's 60px square ground cells.
    # Some are created dynamically and have no authored tilemap instance.
    for object_id in TERRAIN:
        definition = source_objects[object_id]
        original = ROOT / "assets/gfx" / Path(definition[6][0]).name
        size = cell_sizes.get(object_id)
        if size is None and object_id in (482, 687, 688, 776, 980, 1138, 1139):
            size = cell_sizes.get(482, 60)
        if size is None or size <= 0:
            raise SystemExit(f"Unknown source tile size for object {object_id}")
        with Image.open(original) as im:
            if im.width % size or im.height % size > 1:
                raise SystemExit(f"Source tile sheet dimensions changed for {object_id}")
            capacity[object_id] = (im.width // size) * (im.height // size)
    records = []
    counters = Counter()
    for sheet in data[6]:
        for event, ancestors in original_events(sheet[1]):
            for action in event[6] if len(event) > 6 and isinstance(event[6], list) else []:
                if not isinstance(action, list) or len(action) < 6:
                    continue
                object_id, opcode = action[0], action[1]
                if object_id not in TERRAIN:
                    continue
                counters[(object_id, opcode)] += 1
                if opcode != SET_TILE:
                    continue
                params = action[5]
                if not isinstance(params, list) or len(params) < 4:
                    raise SystemExit(f"Malformed source SetTile action {action[3]}")
                expr = params[2][1]
                candidates = tile_choices(expr)
                for tile in candidates:
                    if not 0 <= tile < capacity[object_id]:
                        raise SystemExit(
                            f"Original terrain image {object_id} has {capacity[object_id]} "
                            f"tiles but SetTile action {action[3]} chooses {tile}")
                records.append({
                    "sheet": sheet[0],
                    "object": object_id,
                    "event_uid": event[4],
                    "action_uid": action[3],
                    "ancestor_event_uids": [e[4] for e in ancestors],
                    "conditions": event[5],
                    "parameters": params,
                    "literal_choice_tile_ids": candidates,
                })
    if not records or not any(r["object"] == 230 for r in records):
        raise SystemExit("Missing original dynamic terrain SetTile actions")
    report = {
        "status": "Original source events extracted; native 3DS execution not implied",
        "tile_capacity": capacity,
        "action_counts": {f"{obj}:{opcode}": number
                          for (obj, opcode), number in sorted(counters.items())},
        "set_tile_actions": records,
    }
    DEST.parent.mkdir(parents=True, exist_ok=True)
    DEST.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Source procedural terrain: {len(records)} SetTile actions; "
          f"recorded to {DEST}")


if __name__ == "__main__":
    main()
