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
    # Extract the original pond creation, size, origin, and loop
    # ancestors along with all family range writes. These must be
    # represented in world state, not guessed from static Map RLE.
    event_lookup={}
    for sheet in data[6]:
        for event,ancestors,path in original_events(sheet[1]):
            event_lookup[(sheet[0],path)]=(event,ancestors)
    pond_written=False
    for record in records:
        event,ancestors=event_lookup[(record["sheet"],tuple(record["event_path"]))]
        if record["object"]==208 and record["command"]==SET_TILE_RANGE and not pond_written:
            pond_written=True
            print("STEP1_POND_ROOT",json.dumps({
                "path":record["event_path"],
                "ancestors":[{"uid":n[4],"guards":n[5],"actions":n[6]}
                             for n in ancestors[-3:]+(event,)],
                "children":[{"uid":n[4],"guards":n[5],"actions":n[6]}
                            for n in event[7] if isinstance(n,list) and len(n)>=7 and n[0]==0]
            },separators=(",",":")))
        if record["object"]==GROUND_FAMILY and record["command"]==SET_TILE_RANGE:
            print("STEP1_GROUND_RANGE",json.dumps({
                "path":record["event_path"],"uid":record["action_uid"],
                "params":record["parameters"],
                "guards":record["conditions"],"or":event[2],
                "ancestors":[{"uid":n[4],"guards":n[5],"actions":n[6]}
                             for n in ancestors[-3:]]
            },separators=(",",":")))
    assert pond_written, "Original procedural pond branch missing"
    # Trace the original bridge and startup lifecycle entrypoints and
    # their transitions before replacing the native direct-next-map shortcut.
    for (sheet,path),(event,ancestors) in event_lookup.items():
        if sheet!="Game_events":continue
        def relevant_bridge(a):
            if len(a)<6:return False
            return (a[:2]==[189,69] and a[5][0][1] in (
                [2,"Load_bridge_level"],[2,"Reload_locations_start"],
                [2,"Reload_locations_aftersave"],[2,"mapgen_toelements"])) or (
                a[:2]==[-1,41] and a[5][0] in ([11,"onBridge"],[11,"inRaid"]) and len(event[6])<60)
        if event[4] in (1131020651387518,482054460208135,793361912924261) or any(
                relevant_bridge(a) for a in event[6]):
            print("STEP1_BRIDGE_LIFECYCLE",json.dumps({
                "path":path,"event":event[:7],
                "parents":[{"uid":n[4],"guards":n[5],"actions":n[6]}
                           for n in ancestors[-3:]]
            },separators=(",",":")))
    lifecycle_names={"generate_mapgen_toelements","Reload_locations_start",
                     "Reload_locations","Reload_locations_aftersave","Load_bridge_level"}
    for (sheet,path),(event,ancestors) in event_lookup.items():
        if sheet!="Game_events":continue
        if not any(c[:2]==[189,40] and len(c)>9 and c[9] and
                   c[9][0][1][1] in lifecycle_names for c in event[5]):continue
        name=next(c[9][0][1][1] for c in event[5] if c[:2]==[189,40])
        for child,parents,subpath in original_events([event]):
            print("STEP1_LIFECYCLE_NODE",json.dumps({"name":name,
                  "path":path+subpath[1:],"uid":child[4],"or":child[2],
                  "guards":child[5],"actions":child[6]},separators=(",",":")))
    # Bridge UI, direct entry/exit conditions and original initial object
    # data (including tiled backgrounds, not just Sprite frames).
    for (sheet,path),(event,ancestors) in event_lookup.items():
        if sheet!="Game_events":continue
        if (path[:2]==(476,1) and (len(path)<=3 or path[:3]!=(476,1,3))) or (
                path[:3]==(488,4,10) and len(path)<=4):
            print("STEP1_BRIDGE_TRANSITION",json.dumps({"path":path,
                "uid":event[4],"guards":event[5],"actions":event[6]},
                separators=(",",":")))
    bridge_objects={974,976,977,978,979,348,975,796,318,862,698,484,213,212,
                    487,214,238,239,237,190,260,272,859,860,861,250,574,891}
    for obj in sorted(bridge_objects):
        print("STEP1_BRIDGE_OBJECT",json.dumps({"object":obj,
             "definition":data[3][obj][:8]},separators=(",",":")))
    seen_bridge=set()
    for layout in data[5]:
        for layer in layout[6]:
            for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
                if isinstance(inst,list) and len(inst)>=6 and inst[1] in bridge_objects and inst[1] not in seen_bridge:
                    seen_bridge.add(inst[1])
                    print("STEP1_BRIDGE_DEFAULT",json.dumps({"layout":layout[0],
                        "layer":layer[0],"object":inst[1],"uid":inst[2],
                        "transform":inst[0][:13],"properties":inst[5]},
                        separators=(",",":")))
    other_ids={895209268453600,458228549101251,506106248958134,2084078328497493}
    for record in records:
        if record["action_uid"] not in other_ids:continue
        event,ancestors=event_lookup[(record["sheet"],tuple(record["event_path"]))]
        print("STEP1_OTHER_TERRAIN",json.dumps({"record":record,
            "chain":[{"uid":n[4],"guards":n[5],"actions":n[6],"or":n[2]} for n in (*ancestors,event)]
        },separators=(",",":")))
    for layout in data[5]:
        for layer in layout[6]:
            for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
                if isinstance(inst,list) and len(inst)>=6 and inst[1]==230:
                    print("STEP1_SCATTER_INSTANCE",json.dumps({"layout":layout[0],
                        "layer":layer[0],"uid":inst[2],"transform":inst[0][:13],
                        "size":inst[0][13][:2],"properties":inst[5]},separators=(",",":")))

    # Small source trace with all single writes OTHER than repeating
    # edge predicates, plus ground-instance creation and selection.
    for record in records:
        if record["object"]!=GROUND_FAMILY or record["command"]!=SET_TILE:continue
        event,ancestors=event_lookup[(record["sheet"],tuple(record["event_path"]))]
        if any(c[:2]==[1066,205] for c in event[5]):continue
        print("STEP1_GROUND_SINGLE",json.dumps({
            "path":record["event_path"],"uid":record["action_uid"],
            "params":record["parameters"],"guards":event[5],
            "ancestors":[{"uid":n[4],"guards":n[5],"actions":n[6],"or":n[2]}
                         for n in ancestors[-4:]]
        },separators=(",",":")))
    for (sheet,path),(event,ancestors) in event_lookup.items():
        if sheet!="Game_events":continue
        for a in event[6]:
            if not isinstance(a,list) or len(a)<6:continue
            if (a[:2]==[514,88] and a[5][0]==[10,6]) or (
                a[:2]==[-1,94] and a[5][0][0]==4 and a[5][0][1] in family_members):
                print("STEP1_GROUND_INSTANCE",json.dumps({"path":path,"action":a,
                    "guards":event[5],"siblings":event[6],
                    "ancestors":[{"uid":n[4],"guards":n[5],"actions":n[6]}
                                 for n in ancestors[-3:]]},separators=(",",":")))



if __name__ == "__main__":
    main()
