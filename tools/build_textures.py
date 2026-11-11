#!/usr/bin/env python3
"""Incremental, filename-safe recursive PNG -> Tex3DS converter.

Converts every PNG under the supplied source root, preserving its relative
subdirectory in the output tree. Original artwork is never renamed or modified.
"""
from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


def output_stem(source: Path) -> str:
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

    sources = sorted(
        p for p in input_dir.rglob("*")
        if p.is_file() and p.suffix.lower() == ".png"
    )
    if not sources:
        print(f"no PNG assets found under: {input_dir}", file=sys.stderr)
        return 1

    # Keep source subfolders in the output to avoid cross-folder name clashes.
    owners: dict[Path, Path] = {}
    for source in sources:
        relative = source.relative_to(input_dir)
        relative_output = relative.parent / f"{output_stem(source)}.t3x"
        previous = owners.get(relative_output)
        if previous is not None:
            print(
                f"texture output collision: {previous} and {relative} "
                f"both map to {relative_output}",
                file=sys.stderr,
            )
            return 1
        owners[relative_output] = relative

    output_dir.mkdir(parents=True, exist_ok=True)
    converted = reused = 0
    with tempfile.TemporaryDirectory(prefix="minidayz_tex_") as temp:
        temp_dir = Path(temp)
        for index, source in enumerate(sources):
            relative = source.relative_to(input_dir)
            destination = output_dir / relative.parent / f"{output_stem(source)}.t3x"
            destination.parent.mkdir(parents=True, exist_ok=True)
            if destination.is_file() and destination.stat().st_mtime >= source.stat().st_mtime:
                reused += 1
                continue

            safe_input = temp_dir / f"input_{index:04d}.png"
            safe_output = temp_dir / f"output_{index:04d}.t3x"
            # 3DS hardware textures are limited to 1024x1024. Resize only a
            # temporary conversion copy; never alter the original PNG.
            with Image.open(source) as image:
                image.load()
                if image.width > 1024 or image.height > 1024:
                    image.thumbnail((1024, 1024), Image.Resampling.LANCZOS)
                    image.save(safe_input, format="PNG")
                else:
                    shutil.copyfile(source, safe_input)

            print(f"Building texture: {relative.as_posix()} -> {destination}")
            result = subprocess.run(
                ["tex3ds", "-o", str(safe_output), str(safe_input)],
                check=False,
            )
            if result.returncode != 0 or not safe_output.is_file():
                print(f"tex3ds failed for original asset: {relative}", file=sys.stderr)
                return result.returncode or 1
            shutil.copy2(safe_output, destination)
            converted += 1

    print(
        f"Checked {len(sources)} PNG assets: "
        f"{converted} converted, {reused} unchanged textures reused."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
