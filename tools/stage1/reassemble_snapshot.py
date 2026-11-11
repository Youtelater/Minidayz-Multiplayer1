#!/usr/bin/env python3
"""Reassemble the ordered Stage 1 parsed-source snapshot chunks byte-for-byte."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parts = sorted((root / "data/stage1/source_snapshot").glob("part_*.jsonpart"))
if not parts:
    raise SystemExit("No source snapshot parts found")
target = root / "data/stage1/construct_project_parsed.json"
with target.open("wb") as output:
    for part in parts:
        output.write(part.read_bytes())
print(f"Reassembled {len(parts)} parts into {target.relative_to(root)}")
