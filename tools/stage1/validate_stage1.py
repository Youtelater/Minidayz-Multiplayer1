#!/usr/bin/env python3
"""Fail the Stage 1 pipeline when the source snapshot or generated inventories drift."""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def fail(message: str) -> None:
    raise SystemExit("Stage 1 validation FAILED: " + message)

def read_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"cannot parse {path.relative_to(ROOT)}: {exc}")

def csv_count(path: Path) -> int:
    if not path.is_file():
        fail(f"missing inventory {path.relative_to(ROOT)}")
    with path.open(newline="", encoding="utf-8") as handle:
        return max(0, sum(1 for _ in csv.DictReader(handle)))

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-assets", action="store_true",
                        help="skip checking referenced source PNG files")
    args = parser.parse_args()
    data = ROOT / "data/stage1"
    snapshot = data / "source_snapshot"
    parts = sorted(snapshot.glob("part_*.jsonpart"))
    if not parts:
        fail("no source snapshot chunks found")
    expected = [f"part_{i:03d}.jsonpart" for i in range(len(parts))]
    actual = [p.name for p in parts]
    if actual != expected:
        fail(f"snapshot chunks are missing or misnumbered: {actual[:3]} ... {actual[-3:]}")
    parsed_path = data / "construct_project_parsed.json"
    if not parsed_path.is_file():
        fail("reassembled Construct project is missing")
    rebuilt = b"".join(p.read_bytes() for p in parts)
    parsed_bytes = parsed_path.read_bytes()
    if rebuilt != parsed_bytes:
        fail("reassembled project does not exactly match the ordered source snapshot chunks")

    root = read_json(parsed_path)
    project = root.get("project") if isinstance(root, dict) else None
    if not isinstance(project, list) or len(project) < 17:
        fail("parsed source is not a complete supported Construct project")
    manifest = read_json(data / "stage1_manifest.json")
    index = read_json(data / "event_sheet_index.json")
    checks = {
        "project_sections": len(project),
        "objects": len(project[3]),
        "layouts": len(project[5]),
        "event_sheets": len(project[6]),
        "event_nodes": len(index.get("events", [])),
        "variables": len(index.get("variables", [])),
        "groups": len(index.get("groups", [])),
        "image_frame_references": csv_count(data / "image_refs.csv"),
        "animations": csv_count(data / "animations.csv"),
        "instances": csv_count(data / "instances.csv"),
        "audio_resources": csv_count(data / "audio_resources.csv"),
    }
    for key, actual in checks.items():
        expected_value = manifest.get(key)
        if isinstance(expected_value, list):
            expected_value = len(expected_value)
        if expected_value != actual:
            fail(f"{key}: manifest says {expected_value}, extracted data contains {actual}")

    if len(index.get("sheets", [])) != checks["event_sheets"]:
        fail("event_sheet_index.json does not cover every source event sheet")
    ace_rows = csv_count(data / "ace_reference_inventory.csv")
    ace_pairs = set()
    for call in index.get("ace_calls", []):
        ace_pairs.add((call.get("kind"), call.get("object_index"), call.get("command_id")))
    if ace_rows != len(ace_pairs):
        fail(f"ACE inventory has {ace_rows} rows but event index contains {len(ace_pairs)} unique references")
    group_rows = csv_count(data / "event_groups.csv")
    if group_rows != checks["groups"]:
        fail(f"event_groups.csv has {group_rows} rows but source contains {checks['groups']} named groups")

    header = ROOT / "include/minidayz/generated/stage1_data.hpp"
    if not header.is_file():
        fail("generated C++ Stage 1 header is missing")
    header_text = header.read_text(encoding="utf-8")
    for symbol, key in (("kObjectCount", "objects"), ("kLayoutCount", "layouts"),
                        ("kInstanceCount", "instances"), ("kAnimationCount", "animations"),
                        ("kAudioCount", "audio_resources"), ("kEventCount", "event_nodes")):
        match = re.search(r"\\b" + symbol + r"\\s*=(\\d+)", header_text)
        if not match or int(match.group(1)) != checks[key]:
            fail(f"generated C++ header {symbol} does not match the extracted data")

    if not args.skip_assets:
        assets = ROOT / "assets/gfx"
        if not assets.is_dir():
            fail("assets/gfx source artwork directory is missing")
        refs_path = data / "image_refs.csv"
        with refs_path.open(newline="", encoding="utf-8") as handle:
            refs = list(csv.DictReader(handle))
        by_name = {}
        for path in assets.rglob("*"):
            if path.is_file():
                by_name.setdefault(path.name, []).append(path)
        missing = sorted({row["path"] for row in refs
                          if not by_name.get(Path(row["path"]).name)})
        if missing:
            fail(f"{len(missing)} referenced source image(s) are missing from assets/gfx; examples: {missing[:10]}")

    print("Stage 1 validation PASSED")
    print(json.dumps(checks, indent=2))
    print(f"Verified {len(parts)} ordered snapshot chunks ({len(parsed_bytes):,} bytes).")
    if not args.skip_assets:
        print(f"Verified source artwork for {len(refs)} image-frame references.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
