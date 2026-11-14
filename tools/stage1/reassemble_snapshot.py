#!/usr/bin/env python3
"""Rebuild the parsed-source snapshot from the authoritative Construct export."""
from hashlib import sha256
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = root / "data.deobfuscated.js"
snapshot = root / "data/stage1/source_snapshot"
target = root / "data/stage1/construct_project_parsed.json"
chunk_size = 650_000

if not source.is_file():
    raise SystemExit(f"Authoritative original Construct export is missing: {source.relative_to(root)}")

# The supplied original export is the sole authority. The committed chunks are
# a derived cache and may be stale/incomplete; regenerate them rather than ever
# allowing them to override the original.
original_bytes = source.read_bytes()
snapshot.mkdir(parents=True, exist_ok=True)

expected_count = (len(original_bytes) + chunk_size - 1) // chunk_size
for index in range(expected_count):
    start = index * chunk_size
    chunk = original_bytes[start:start + chunk_size]
    (snapshot / f"part_{index:03d}.jsonpart").write_bytes(chunk)

for stale_part in snapshot.glob("part_*.jsonpart"):
    try:
        index = int(stale_part.stem.split("_")[1])
    except (IndexError, ValueError):
        continue
    if index >= expected_count:
        stale_part.unlink()

parts = sorted(snapshot.glob("part_*.jsonpart"))
expected_names = [f"part_{index:03d}.jsonpart" for index in range(expected_count)]
if [part.name for part in parts] != expected_names:
    raise SystemExit("Regenerated source snapshot is missing or misnumbered")

snapshot_bytes = b"".join(part.read_bytes() for part in parts)
if snapshot_bytes != original_bytes:
    raise SystemExit("Internal error: regenerated snapshot does not match data.deobfuscated.js")

target.write_bytes(original_bytes)
print(
    f"Rebuilt {len(parts)} snapshot chunks directly from authoritative "
    f"data.deobfuscated.js ({len(original_bytes):,} bytes, "
    f"sha256={sha256(original_bytes).hexdigest()})"
)
print(f"Wrote exact source to {target.relative_to(root)}")
