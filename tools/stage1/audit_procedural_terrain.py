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
import struct

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data.deobfuscated.js"
DEST = ROOT / "data/stage1/original_terrain_paint_events.json"
# Four environment objects plus the seven original per-level ground objects.
# The Construct runtime can paint BOTH sets with SetTile actions; excluding
# the ground sheets silently hides the largest source-parity gap.
TERRAIN = (208, 211, 418, 230, 482, 687, 688, 776, 980, 1138, 1139)
# Original Construct Tilemap 'Set tile at' opcode, confirmed from root source.
SET_TILE = 192
SET_TILE_RANGE = 202
GROUND_FAMILY = 1066


def original_events(events, ancestors=(), path=()):
    for index, event in enumerate(events):
        if not isinstance(event, list) or not event or event[0] != 0:
            continue
        location = path + (index,)
        yield event, ancestors, location
        children = event[7] if len(event) > 7 and isinstance(event[7], list) else ()
        yield from original_events(children, ancestors + (event,), location)


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
    for child in tree:
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
        # Audit the exact source PNG's IHDR dimensions without requiring
        # Pillow or altering an original image / generated atlas.
        with original.open("rb") as stream:
            ihdr=stream.read(24)
        if (len(ihdr)!=24 or ihdr[:8]!=bytes((137,80,78,71,13,10,26,10))
                or ihdr[12:16]!=b"IHDR"):
            raise SystemExit(f"Bad original source PNG for {object_id}")
        width,height=struct.unpack(">II",ihdr[16:24])
        if width<=0 or height<=0 or width%size or height%size>1:
            raise SystemExit(f"Source tile sheet dimensions changed for {object_id}")
        capacity[object_id]=(width//size)*(height//size)
    family_members = next(row[1:] for row in data[4] if row[0] == GROUND_FAMILY)
    if set(family_members) != set(TERRAIN[4:]):
        raise SystemExit("Original ground tilemap family changed")
    records = []
    guard_events = {}
    location_loaders = {}
    counters = Counter()
    for sheet in data[6]:
        for event, ancestors, path in original_events(sheet[1]):
            if any(c[:2] == [189, 40] and c[-1] == [[1, [2, "create_map_location"]]]
                   for c in event[5]):
                location_loaders.setdefault(sheet[0], []).append({"path": path, "event": event})
            for action_index, action in enumerate(event[6] if len(event) > 6 else []):
                if not isinstance(action, list) or len(action) < 6:
                    continue
                object_id, opcode = action[0], action[1]
                if object_id not in capacity and object_id != GROUND_FAMILY:
                    continue
                counters[(object_id, opcode)] += 1
                if opcode not in (SET_TILE, SET_TILE_RANGE):
                    continue
                params = action[5]
                tile_parameter = 2 if opcode == SET_TILE else 4
                if not isinstance(params, list) or len(params) != tile_parameter + 2:
                    raise SystemExit(f"Malformed tile paint action {action[3]}")
                expr = params[tile_parameter][1]
                candidates = [expr[1]] if expr[0] == 0 else tile_choices(expr)
                targets = family_members if object_id == GROUND_FAMILY else [object_id]
                for tile in candidates:
                    for target in targets:
                        if not 0 <= tile < capacity[target]:
                            raise SystemExit(f"Tile {tile} outside original sheet {target}")
                # Retain the full event headers and their order. In particular,
                # an OR block with two marker codes must never become an AND.
                sheet_headers = guard_events.setdefault(sheet[0], {})
                for depth, node in enumerate((*ancestors, event), start=1):
                    key = ".".join(map(str, path[:depth]))
                    sheet_headers.setdefault(key, node[:6])
                records.append({
                    "command": opcode,
                    "event_path": path,
                    "action_index": action_index,
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
    # The seven original ground-theme family1066 paints must ALL remain in
    # the Step 1 evidence manifest, including source event paths/guards.
    if (counters[GROUND_FAMILY,SET_TILE] != 398 or
            counters[GROUND_FAMILY,SET_TILE_RANGE] != 163):
        raise SystemExit("Original ground paint action count changed: "
                         f"{counters[GROUND_FAMILY,SET_TILE]}/"
                         f"{counters[GROUND_FAMILY,SET_TILE_RANGE]}")
    report = {
        "status": "Original source events extracted; native 3DS execution not implied",
        "ground_family_members": family_members,
        "event_headers": guard_events,
        "location_loaders": location_loaders,
        "tile_capacity": capacity,
        "action_counts": {f"{obj}:{opcode}": number
                          for (obj, opcode), number in sorted(counters.items())},
        "set_tile_actions": records,
    }
    DEST.parent.mkdir(parents=True, exist_ok=True)
    DEST.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Source procedural terrain: {len(records)} SetTile actions; "
          f"recorded to {DEST}")
    # Compact coverage information for implementation, not a fabricated
    # execution count. Include both the complete original function/marker
    # ancestry and the exact action and condition opcode profiles.
    from collections import defaultdict
    by_scope=defaultdict(list)
    for record in records:
        sheet=record["sheet"]
        headers=guard_events[sheet]
        path=record["event_path"]
        chain=[headers[".".join(map(str,path[:d]))] for d in range(1,len(path)+1)]
        function="<sheet>"
        marker=[]
        for node in chain:
            for condition in node[5]:
                if len(condition)>9 and condition[:2]==[189,40]:
                    params=condition[9]
                    if params and params[0][0]==1:
                        function=params[0][1][1]
                if len(condition)>9 and condition[:2]==[514,71]:
                    params=condition[9]
                    if len(params)==3 and params[0]==[10,0]:
                        marker.append(params[2])
        key=(sheet,function,json.dumps(marker,separators=(",",":")))
        by_scope[key].append(record)
    for (sheet,function,marker),scope in by_scope.items():
        profile=Counter(f"{r['object']}:{r['command']}" for r in scope)
        leaf=Counter(f"{c[0]}:{c[1]}" for r in scope for c in r["conditions"])
        print("STEP1_TERRAIN_SCOPE",json.dumps({
            "sheet":sheet,"function":function,"marker":json.loads(marker),
            "count":len(scope),"actions":dict(profile),"leaf_conditions":dict(leaf)
        },separators=(",",":")))
    # All non-ground environment writes: exact expressions and direct
    # conditions from original data, with full paths for ancestor lookups.
    for record in records:
        if record["object"]!=GROUND_FAMILY:
            print("STEP1_ENVIRONMENT_PAINT",json.dumps(record,separators=(",",":")))



if __name__ == "__main__":
    main()
