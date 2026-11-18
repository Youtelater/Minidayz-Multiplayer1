#!/usr/bin/env python3
"""Index all canonical original PNGs by filename for 3DS runtime loading.

The ordinary Construct sprite-frame catalog covers animated frames. This
companion index makes all original still-image sheets addressable, including
assets not referenced by Construct animations. No invented replacements.
"""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "assets/gfx"
DEST = ROOT / "include/minidayz/generated/original_image_catalog.hpp"


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
    outputs = {}
    for source in sources:
        target = convert(Path(source))
        if target in outputs:
            raise SystemExit(f"3DS output collision: {source} and {outputs[target]}")
        outputs[target] = source
        artifact = ROOT / "romfs/gfx" / target
        if not artifact.is_file() or artifact.stat().st_size <= 32:
            raise SystemExit(f"Unconverted original image: {source} ({target})")
    text = [
        "#pragma once",
        "#include <cstddef>",
        "namespace minidayz::stage1 {",
        "struct OriginalImageAsset { const char* png; const char* texture; };",
        f"inline constexpr std::size_t kOriginalImageCount={len(sources)};",
        "inline constexpr OriginalImageAsset kOriginalImages[] = {",
    ]
    for source in sources:
        text.append(f"  {{{json.dumps(source, ensure_ascii=False)},"
                    f"{json.dumps(convert(Path(source)), ensure_ascii=False)}}},")
    text.extend(["};", "} // namespace minidayz::stage1", ""])
    DEST.parent.mkdir(parents=True, exist_ok=True)
    DEST.write_text("\n".join(text), encoding="utf-8")
    print(f"Indexed {len(sources)} original PNG names and matching 3DS textures")


if __name__ == "__main__":
    main()
