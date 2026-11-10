#!/usr/bin/env python3
"""Create a source-backed index of named Construct event groups.
Run from the repository root:
  python tools/stage1/audit_event_groups.py data.deobfuscated.js
Reads only the supplied Construct export; never reads romfs/, raw/, c2injt.js,
or multiplayer JSON files.
"""
import argparse
import csv
import json
import re
from pathlib import Path

def load_project(path):
    source = Path(path).read_text(encoding="utf-8")
    source = re.sub(r'(?<![A-Za-z0-9_.])-\\s+(?=\\d)', '-', source)
    root = json.loads(source)
    project = root.get("project") if isinstance(root, dict) else None
    if not isinstance(project, list) or len(project) < 7:
        raise ValueError("Not a supported Construct project export")
    return project

def event_children(event):
    children = event[7] if len(event) > 7 and isinstance(event[7], list) else []
    return [child for child in children if isinstance(child, list) and child and child[0] == 0]

def walk_events(events):
    for event in events:
        if not isinstance(event, list) or not event or event[0] != 0:
            continue
        yield event
        yield from walk_events(event_children(event))

def group_name(event):
    label = event[1] if len(event) > 1 else None
    if isinstance(label, list) and len(label) > 1 and isinstance(label[1], str):
        return label[1]
    return None

def summarize(event):
    nodes = conditions = actions = 0
    for node in walk_events([event]):
        nodes += 1
        conditions += len(node[5]) if len(node) > 5 and isinstance(node[5], list) else 0
        actions += len(node[6]) if len(node) > 6 and isinstance(node[6], list) else 0
    return nodes, conditions, actions

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", nargs="?", default="data.deobfuscated.js")
    parser.add_argument("--out", default="data/stage1/event_groups.csv")
    args = parser.parse_args()
    project = load_project(args.source)
    rows = []
    for sheet_index, sheet in enumerate(project[6]):
        sheet_name, entries = sheet
        for event in walk_events(entries):
            name = group_name(event)
            if name is None:
                continue
            nodes, conditions, actions = summarize(event)
            rows.append({
                "sheet_index": sheet_index,
                "sheet": sheet_name,
                "name": name,
                "uid": event[4] if len(event) > 4 else 0,
                "nested_event_nodes": nodes,
                "condition_entries": conditions,
                "action_entries": actions,
            })
    output = Path(args.out)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]) if rows else [
            "sheet_index", "sheet", "name", "uid", "nested_event_nodes",
            "condition_entries", "action_entries"])
        writer.writeheader()
        writer.writerows(rows)
    print(json.dumps({"named_group_declarations": len(rows), "output": str(output)}, indent=2))

if __name__ == "__main__":
    main()
