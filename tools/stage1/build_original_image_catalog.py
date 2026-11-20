#!/usr/bin/env python3
"""Index all canonical original PNGs by filename for 3DS runtime loading.

The ordinary Construct sprite-frame catalog covers animated frames. This
companion index makes all original still-image sheets addressable, including
assets not referenced by Construct animations. No invented replacements.
"""
from __future__ import annotations

import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "assets/gfx"
DEST = ROOT / "include/minidayz/generated/original_image_catalog.hpp"
PARSED = ROOT / "data/stage1/construct_project_parsed.json"


def canonical(filename: str) -> str:
    return Path(filename.replace("\\", "/")).name.casefold()


def source_atlas_metadata():
    """Source-confirmed multi-frame sprites and Construct tilemap textures."""
    if not PARSED.is_file():
        raise SystemExit("Missing original parsed Construct export for atlas safety")
    source = re.sub(r"(?<![A-Za-z0-9_.])-\\s+(?=\\d)", "-", PARSED.read_text(encoding="utf-8-sig"))
    objects = json.loads(source)["project"][3]
    rects = defaultdict(set)
    tilemaps = set()
    for obj in objects:
        if not isinstance(obj, list):
            continue
        if len(obj) > 6 and isinstance(obj[6], list):
            for value in obj[6]:
                if isinstance(value, str) and value.lower().endswith(".png"):
                    tilemaps.add(canonical(value))
        for anim in (obj[7] if len(obj) > 7 and isinstance(obj[7], list) else []):
            if not isinstance(anim, list) or len(anim) < 8 or not isinstance(anim[7], list):
                continue
            for frame in anim[7]:
                if not isinstance(frame, list) or len(frame) < 6 or not isinstance(frame[0], str):
                    continue
                rects[canonical(frame[0])].add(tuple(int(frame[i]) for i in (2, 3, 4, 5)))
    return rects, tilemaps


def convert(relative: Path) -> str:
    return relative.with_name(
        relative.stem.replace("-", "_").replace(".", "_") + ".t3x"
    ).as_posix()


def main() -> None:
    sources = sorted(
        p.relative_to(SRC).as_posix()
        for p in SRC.rglob("*") if p.is_file() and p.suffix.casefold() == ".png"
    )
    if not sources:
        raise SystemExit("No original images in assets/gfx")
    rects, tilemaps = source_atlas_metadata()
    outputs = {}
    for source in sources:
        target = convert(Path(source))
        if target in outputs:
            raise SystemExit(f"3DS output collision: {source} and {outputs[target]}")
        outputs[target] = source
        artifact = ROOT / "romfs/gfx" / target
        if not artifact.is_file() or artifact.stat().st_size == 0:
            raise SystemExit(f"Unconverted original image: {source} ({target})")
    text = [
        "#pragma once",
        "#include <cstddef>",
        "namespace minidayz::stage1 {",
        "struct OriginalImageAsset { const char* png; const char* texture; bool is_source_atlas; };",
        f"inline constexpr std::size_t kOriginalImageCount={len(sources)};",
        "inline constexpr OriginalImageAsset kOriginalImages[] = {",
    ]
    for source in sources:
        # A Construct animation sheet with several different frame rectangles
        # or a tilemap is NEVER one safely drawable full-image sprite.
        name = canonical(source)
        atlas = len(rects.get(name, ())) > 1 or name in tilemaps
        text.append(f"  {{{json.dumps(source, ensure_ascii=False)},"
                    f"{json.dumps(convert(Path(source)), ensure_ascii=False)},"
                    f"{str(atlas).lower()}}},")
    text.extend(["};", "} // namespace minidayz::stage1", ""])
    DEST.parent.mkdir(parents=True, exist_ok=True)
    DEST.write_text("\n".join(text), encoding="utf-8")
    print(f"Indexed {len(sources)} original PNG names and matching 3DS textures; "
          f"{sum(len(rects.get(canonical(src), ())) > 1 or canonical(src) in tilemaps for src in sources)} "
          "source-confirmed multi-frame or tilemap atlases cannot be stretched whole")


if __name__ == "__main__":
    main()
