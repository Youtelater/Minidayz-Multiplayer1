#!/usr/bin/env python3
"""Verify the committed source snapshot against data.deobfuscated.js, then reassemble it."""
from hashlib import sha256
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = root / "data.deobfuscated.js"
snapshot = root / "data/stage1/source_snapshot"
target = root / "data/stage1/construct_project_parsed.json"

if not source.is_file():
    raise SystemExit(f"Authoritative original Construct export is missing: {source.relative_to(root)}")

parts = sorted(snapshot.glob("part_*.jsonpart"))
if not parts:
    raise SystemExit("No source snapshot parts found")

expected_names = [f"part_{index:03d}.jsonpart" for index in range(len(parts))]
actual_names = [part.name for part in parts]
if actual_names != expected_names:
    raise SystemExit("Source snapshot chunks are missing or misnumbered")

# Compare raw bytes, not parsed/re-serialized JSON: whitespace, numeric spelling,
# event order, and every source record must remain exactly as supplied.
original_bytes = source.read_bytes()
snapshot_bytes = b"".join(part.read_bytes() for part in parts)
if original_bytes != snapshot_bytes:
    original_hash = sha256(original_bytes).hexdigest()
    snapshot_hash = sha256(snapshot_bytes).hexdigest()
    raise SystemExit(
        "AUTHORITATIVE SOURCE MISMATCH: data.deobfuscated.js does not exactly "
        "match the committed snapshot. Refusing to generate gameplay data. "
        f"original_bytes={len(original_bytes)} snapshot_bytes={len(snapshot_bytes)} "
        f"original_sha256={original_hash} snapshot_sha256={snapshot_hash}"
    )

target.write_bytes(snapshot_bytes)
print(
    f"Verified original data.deobfuscated.js against {len(parts)} snapshot chunks "
    f"({len(snapshot_bytes):,} bytes, sha256={sha256(snapshot_bytes).hexdigest()})"
)
print(f"Reassembled exact authoritative source into {target.relative_to(root)}")
