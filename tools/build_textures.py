#!/usr/bin/env python3
"""Incremental, filename-safe PNG -> Tex3DS converter.

Original assets are never renamed or modified. Tex3DS receives a short ASCII
temporary filename so spaces and non-ASCII characters in source paths cannot
be misparsed by its image-loading dependencies.
"""
from __future__ import annotations

import hashlib
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def output_stem(source: Path) -> str:
    # Keep the established runtime naming convention.
    return source.stem.replace("-", "_").replace(".", "_")


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: build_textures.py INPUT_DIR OUTPUT_DIR", file=sys.stderr)
        return 2

    input_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    if not input_dir.is_dir():
        print(f"graphics input directory not found: {input_dir}", file=sys.stderr)
        return 2

    output_dir.mkdir(parents=True, exist_ok=True)
    sources = sorted(p for p in input_dir.iterdir()
                     if p.is_file() and p.suffix.lower() == ".png")

    # Detect names that would overwrite one another after the Makefile's
    # historical hyphen/dot normalization rather than silently losing art.
    owners: dict[str, Path] = {}
    for source in sources:
        stem = output_stem(source)
        previous = owners.get(stem)
        if previous is not None:
            print(f"texture output collision: {previous.name!r} and "
                  f"{source.name!r} both map to {stem}.t3x", file=sys.stderr)
            return 1
        owners[stem] = source

    with tempfile.TemporaryDirectory(prefix="minidayz_tex_") as temp:
        temp_dir = Path(temp)
        for index, source in enumerate(sources):
            destination = output_dir / f"{output_stem(source)}.t3x"
            if destination.exists() and destination.stat().st_mtime >= source.stat().st_mtime:
                continue

            safe_input = temp_dir / f"input_{index:04d}.png"
            safe_output = temp_dir / f"output_{index:04d}.t3x"
            shutil.copyfile(source, safe_input)

            print(f"Building texture sheet: {destination.name}")
            result = subprocess.run(
                ["tex3ds", "-o", str(safe_output), str(safe_input)],
                check=False,
            )
            if result.returncode != 0 or not safe_output.is_file():
                print(f"tex3ds failed for original asset: {source.name!r}",
                      file=sys.stderr)
                return result.returncode or 1
            shutil.copy2(safe_output, destination)

    print(f"Checked {len(sources)} PNG assets; unchanged textures were reused.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
