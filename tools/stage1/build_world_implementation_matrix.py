#!/usr/bin/env python3
"""Derive an actionable, lossless world event matrix from root-source traces.

Run audit_original_world_exhaustive.py first. This script never uses cached
event_groups.csv, derived C++ tables, romfs, or an alternate original version.
Numeric Construct commands are kept as IDs: names and effects are unverified
until validated against c2runtime.js and their original plugin implementations.
"""
import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATEGORIES = {
    "generation": ("map generation", "location loader", "bunker_generate", "bridge level",
                   "minimap_legend", "seed td", "seed runs", "level_change", "tutorial_map"),
    "terrain_and_foliage": ("buildings and obstacles", "obstacles_transparenting",
                            "big_obstacles_transparenting", "hitting_obstacles",
                            "range_destroy", "range_less_power", "gardening",
                            "barberwire", "building mode", "envir_effects"),
    "buildings_and_doors": ("house_transparenting", "doors_auto", "building mode",
                            "bunker_generate", "buildings and obstacles"),
    "items_and_spawning": ("spawn parameters", "loot respawn", "picking player items",
                           "picking inventory items", "droppin items", "pickpile",
                           "merged pick and interact", "loot_checker2", "stash", "airdrop"),
    "living_world": ("npcs", "zombies", "zeds_common", "z_spawners", "npc_manager",
                     "spawn_npc", "bots", "bot manager", "animals", "wolf",
                     "damage dealing", "bullet", "beartrap", "molotov_damage"),
    "vehicles_and_travel": ("drivable vehicles", "vehicle collisions", "vehicle layering",
                            "heli", "bridge level"),
    "weather_and_time": ("time cyles and weather", "day&night", "rain",
                         "light parameters", "light webgl", "fireplace params", "sleeping"),
    "progression_and_persistence": ("quests", "level_change", "final_day",
                                     "tutorial", "sav", "building mode", "stash"),
}
# These are *source event action signatures* seen in the extracted Construct
# arrays, not a general mapping from numeric ID to gameplay semantics.
AUDIO_SIGNATURES = {(187, 81), (187, 398)}
VISUAL_SIGNATURES = {72, 132, 137, 163}
FUNCTION_CALL_SIGNATURE = (189, 69)
FUNCTION_DEFINE_SIGNATURE = (189, 40)
FOCAL_OBJECT_IDS = {184, 207, 209, 210, 296, 319, 321, 322, 323, 324,
                    333, 349, 576, 577, 581, 787, 1048, 1049, 218}


def literal_strings(obj):
    out = []
    def walk(value):
        if isinstance(value, list):
            if len(value) == 2 and value[0] == 2 and isinstance(value[1], str):
                out.append(value[1])
            else:
                for part in value:
                    walk(part)
    walk(obj)
    return out


def event_classification(groups):
    lowered = [g.lower() for g in groups]
    matched = []
    for category, tokens in CATEGORIES.items():
        if any(any(token in group for token in tokens) for group in lowered):
            matched.append(category)
    return matched


def tocsv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--in-dir", type=Path, default=ROOT / "audit-output/world")
    args = parser.parse_args()
    src = args.in_dir / "all_original_event_traces.jsonl"
    if not src.is_file():
        raise SystemExit("Run audit_original_world_exhaustive.py first")
    source_summary = json.loads((args.in_dir / "summary.json").read_text())
    events = []
    conditions_by_source_path = {}
    definitions = defaultdict(list)
    calls = defaultdict(list)
    count_by_category = Counter()
    conditions_by_object = defaultdict(Counter)
    actions_by_object = defaultdict(Counter)
    action_features = []
    by_uid = defaultdict(list)
    for line in src.open(encoding="utf-8"):
        e = json.loads(line)
        path = e["sheet"] + ":" + ".".join(map(str, e["source_event_path"]))
        cats = event_classification(e["groups"])
        uid = e["uid"]
        by_uid[uid].append(path)
        all_conditions = e["conditions"]
        # Parent nodes have already been emitted by the depth-first source
        # trace. Preserve their exact condition expressions, including event
        # modifiers; an action's own conditions alone are insufficient.
        event_indices = e["source_event_path"]
        ancestor_paths = [
            e["sheet"] + ":" + ".".join(map(str, event_indices[:n]))
            for n in range(1, len(event_indices))
        ]
        inherited = [
            {"path": parent, "conditions": conditions_by_source_path[parent]["conditions"],
             "flags": conditions_by_source_path[parent]["flags"]}
            for parent in ancestor_paths if parent in conditions_by_source_path
        ]
        conditions_by_source_path[path] = {
            "conditions": all_conditions, "flags": e["flags"]
        }
        all_actions = e["actions"]
        objects = sorted({x[0] for x in all_conditions + all_actions
                          if isinstance(x, list) and x and isinstance(x[0], int)
                          and x[0] >= 0})
        sound_literals = []
        effects = []
        function_names = []
        for i, cond in enumerate(all_conditions):
            if isinstance(cond, list) and len(cond) > 9:
                if tuple(cond[:2]) == FUNCTION_DEFINE_SIGNATURE:
                    for name in literal_strings(cond[9:]):
                        definitions[name].append(path)
                if isinstance(cond[0], int) and cond[0] >= 0 and len(cond) > 1:
                    conditions_by_object[cond[0]][cond[1]] += 1
        for i, action in enumerate(all_actions):
            if not isinstance(action, list) or len(action) < 2:
                continue
            obj, cmd = action[:2]
            if isinstance(obj, int) and obj >= 0:
                actions_by_object[obj][cmd] += 1
            literal = literal_strings(action[5:])
            if (obj, cmd) == FUNCTION_CALL_SIGNATURE:
                name = literal[0] if literal else "<dynamic>"
                calls[name].append(path)
                function_names.append(name)
                effects.append("function_call")
            if (obj, cmd) in AUDIO_SIGNATURES:
                sound_literals.extend(literal)
                effects.append("source_audio_action")
            if obj != -1 and cmd in VISUAL_SIGNATURES:
                effects.append("candidate_visual_action")
            if obj in FOCAL_OBJECT_IDS:
                effects.append("focal_object_action")
            if effects and (cats or obj in FOCAL_OBJECT_IDS):
                action_features.append({
                    "source_event_path": path,
                    "source_event_uid": uid,
                    "groups": "/".join(e["groups"]),
                    "object_id": obj,
                    "command_id": cmd,
                    "action_index": i,
                    "literals_json": json.dumps(literal, ensure_ascii=False),
                    "action_json": json.dumps(action, ensure_ascii=False, separators=(",", ":")),
                    "categories": "|".join(cats),
                })
                # Reset so the flags of one action cannot leak into the next.
                effects = []
        for cat in cats:
            count_by_category[cat] += 1
        if cats or set(objects) & FOCAL_OBJECT_IDS or sound_literals:
            events.append({
                "source_event_path": path,
                "source_event_uid": uid,
                "categories": "|".join(cats),
                "groups": "/".join(e["groups"]),
                "ancestor_uids": "|".join(map(str, e["parent_uids"])),
                "event_flags_json": json.dumps(e["flags"], ensure_ascii=False),
                "inherited_condition_chain_json": json.dumps(inherited, ensure_ascii=False, separators=(",", ":")),
                "condition_count": len(all_conditions),
                "action_count": len(all_actions),
                "object_ids": "|".join(map(str, objects)),
                "source_audio_literals": "|".join(sound_literals),
                "function_calls": "|".join(function_names),
                "conditions_json": json.dumps(all_conditions, ensure_ascii=False, separators=(",", ":")),
                "actions_json": json.dumps(all_actions, ensure_ascii=False, separators=(",", ":")),
            })
    edge_rows = []
    unresolved = []
    for name, call_paths in sorted(calls.items()):
        targets = definitions.get(name, [])
        if not targets:
            unresolved.append(name)
        for source in call_paths:
            edge_rows.append({
                "call_source_path": source,
                "called_literal": name,
                "definition_count": len(targets),
                "definition_source_paths": "|".join(targets),
                "resolved_statically": bool(targets),
            })
    tocsv(args.in_dir / "world_event_matrix.csv",
          ["source_event_path", "source_event_uid", "categories", "groups",
           "ancestor_uids", "event_flags_json", "inherited_condition_chain_json",
           "condition_count", "action_count", "object_ids",
           "source_audio_literals", "function_calls", "conditions_json", "actions_json"], events)
    tocsv(args.in_dir / "world_function_edges.csv",
          ["call_source_path", "called_literal", "definition_count",
           "definition_source_paths", "resolved_statically"], edge_rows)
    tocsv(args.in_dir / "world_action_effect_candidates.csv",
          ["source_event_path", "source_event_uid", "groups", "object_id",
           "command_id", "action_index", "literals_json", "action_json", "categories"],
          action_features)
    tocsv(args.in_dir / "world_object_command_inventory.csv",
          ["object_id", "condition_commands_json", "action_commands_json"],
          [{"object_id": i,
            "condition_commands_json": json.dumps(dict(conditions_by_object.get(i, {})), sort_keys=True),
            "action_commands_json": json.dumps(dict(actions_by_object.get(i, {})), sort_keys=True)}
           for i in sorted(set(conditions_by_object) | set(actions_by_object))])
    summary = {
        "source": source_summary["source"],
        "source_sha256": source_summary["provenance"]["source_sha256"],
        "total_original_events": source_summary["events"],
        "world_and_crosscutting_event_rows": len(events),
        "category_event_counts_overlapping": dict(count_by_category),
        "world_action_effect_candidates": len(action_features),
        "function_call_edges": len(edge_rows),
        "function_definitions": len(definitions),
        "functions_unresolved_statically": unresolved,
        "event_uid_duplicates": {str(k): v for k, v in by_uid.items() if len(v) > 1},
        "critical_next_verification": [
            "Resolve numeric command IDs against the original Construct runtime and plugins",
            "Trace complete inherited condition and expression evaluations",
            "Verify runtime function call order and RNG state",
            "Prove object assets against root original, not old cached snapshot",
            "Compare reconstructed world outputs with original running behavior",
        ],
    }
    (args.in_dir / "world_implementation_matrix_summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    md = [
        "# Original MiniDAYZ — root-source world implementation matrix",
        "",
        f"Original source SHA256: \`{summary['source_sha256']}\`",
        f"Parsed events (all sheets): **{summary['total_original_events']:,}**.",
        f"World and cross-cutting event rows: **{len(events):,}**.",
        f"Candidate interaction action rows: **{len(action_features):,}**.",
        f"Literal function call edges: **{len(edge_rows):,}**.",
        "",
        "## Overlapping source-group categories",
        "",
        "| Category | Event rows |", "|---|---:|",
    ]
    md += [f"| {k} | {v:,} |" for k, v in sorted(count_by_category.items())]
    md += [
        "",
        "## Machine-readable outputs",
        "",
        "- \`all_original_event_traces.jsonl\`: all source events, exact condition/action arrays, group ancestors and unique source paths.",
        "- \`world_event_matrix.csv\`: source-authored event flags, every inherited parent condition, and original action arrays.",
        "- \`world_function_edges.csv\`: call sites, literal function names, and matched definition paths.",
        "- \`world_action_effect_candidates.csv\`: event-target action signatures, not inferred gameplay effects.",
        "- \`world_object_command_inventory.csv\`: exact condition/action command frequency for every referenced original object.",
        "- \`audio_actions.csv\`, \`original_sprite_frames.csv\`, \`original_png_accounting.csv\`: the original evidence tables.",
        "",
        "## Interpretation limits",
        "",
        "**Source syntax coverage is not interpreted gameplay parity.** The scanner does not claim to decode every numeric Construct command or dynamic function name, prove all source paths are reachable, or certify the 3DS implementation. Cross-cutting event groups are included, but source functions called from them require full transitive tracing during implementation.",
        "Preserve original JS expressions and source UID *and* event path. A duplicated UID does not uniquely identify an event.",
        "Do not promote candidate audio, visuals, rewards, or collision to a gameplay requirement until their original conditions and runtime commands are verified.",
        "",
    ]
    (args.in_dir / "WORLD_IMPLEMENTATION_MATRIX.md").write_text(
        "\n".join(md), encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k != "event_uid_duplicates"},
                     indent=2, ensure_ascii=False))
    if source_summary["events"] < 1000 or len(events) < 1000:
        raise SystemExit("Unexpectedly few original/world events; refuse incomplete audit")


if __name__ == "__main__":
    main()
