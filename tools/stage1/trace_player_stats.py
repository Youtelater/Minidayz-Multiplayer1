#!/usr/bin/env python3
"""Export the original single-player Player stats event tree for source tracing.

Run from the repository root:
  python tools/stage1/trace_player_stats.py

Input is the parsed Construct export produced by extract_construct.py.
This tool intentionally reads only that parsed export; it never scans raw/,
romfs/, c2injt.js, MP JSON, or the restricted Minidayz-Multiplayer-main folder.
"""
import argparse
import csv
import json
from pathlib import Path


def walk(items, path=(), parent_uid=0, depth=0):
    for event in items if isinstance(items, list) else []:
        if not isinstance(event, list) or not event or event[0] != 0:
            continue
        uid = event[4] if len(event) > 4 and isinstance(event[4], int) else 0
        header = event[1] if len(event) > 1 else None
        group_name = header[1] if isinstance(header, list) and len(header) > 1 and isinstance(header[1], str) else ""
        conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
        actions = event[6] if len(event) > 6 and isinstance(event[6], list) else []
        children = event[7] if len(event) > 7 and isinstance(event[7], list) else []
        row = {
            "uid": uid, "parent_uid": parent_uid, "depth": depth,
            "group": group_name, "conditions": conditions, "actions": actions,
            "child_count": len(children),
        }
        yield row
        yield from walk(children, path + (uid,), uid, depth + 1)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", default="data/stage1/construct_project_parsed.json")
    parser.add_argument("--out", default="data/stage1/player_stats_trace")
    args = parser.parse_args()

    root = json.loads(Path(args.input).read_text(encoding="utf-8"))
    project = root.get("project")
    if not isinstance(project, list) or len(project) <= 6:
        raise SystemExit("Input is not a parsed Construct project export")

    target_uid = 7463747062628312
    found = []
    for sheet_name, entries in project[6]:
        if sheet_name != "Game_events":
            continue
        all_rows = list(walk(entries))
        # Include the group node itself and all descendants, identified by the
        # source event's group marker. Child rows inherit the active group.
        active = False
        for event in entries:
            if not isinstance(event, list) or not event or event[0] != 0:
                continue
            h = event[1] if len(event) > 1 else None
            name = h[1] if isinstance(h, list) and len(h) > 1 else ""
            uid = event[4] if len(event) > 4 else 0
            if uid == target_uid or name == "Player stats":
                active = True
                found.extend(walk([event]))
                break
        if active:
            break

    if not found:
        raise SystemExit("Player stats group UID not found in Game_events")

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    json_path = out / "events.json"
    csv_path = out / "events.csv"
    json_path.write_text(json.dumps(found, ensure_ascii=False, indent=2), encoding="utf-8")
    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=[
            "uid", "parent_uid", "depth", "group", "conditions", "actions", "child_count"
        ])
        writer.writeheader()
        for row in found:
            writer.writerow({**row,
                "conditions": json.dumps(row["conditions"], ensure_ascii=False),
                "actions": json.dumps(row["actions"], ensure_ascii=False)})
    print(f"Wrote {len(found)} source event records to {json_path} and {csv_path}")


if __name__ == "__main__":
    main()
