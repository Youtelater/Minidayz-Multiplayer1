#!/usr/bin/env python3
"""Verify every original MiniDAYZ PNG has its committed 3DS texture.

Also verify all Construct animation frames, player sprites and original item
sprite references resolve to generated Tex3DS atlases. A packaged image is
not automatically a drawn image: gameplay-event coverage is tracked separately.
Do not source duplicate images/ assets.
"""
from __future__ import annotations

import re
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "assets" / "gfx"
TEXTURES = ROOT / "romfs" / "gfx"
SPRITES = ROOT / "include/minidayz/generated/sprite_catalog.hpp"
ITEMS = ROOT / "include/minidayz/generated/item_sprite_catalog.hpp"
ALL_IMAGES = ROOT / "include/minidayz/generated/original_image_catalog.hpp"

FRAME = re.compile(
    r'^\{(\d+),(\d+),(\d+),(\d+),"([^"]+)",', re.MULTILINE
)
ITEM = re.compile(r'^\s*\{(\d+),\s*(\d+)\},', re.MULTILINE)


def mapped_name(relative: Path) -> Path:
    stem = relative.stem.replace("-", "_").replace(".", "_")
    return relative.with_name(stem + ".t3x")


def check() -> None:
    if not ORIGINAL.is_dir() or not TEXTURES.is_dir():
        raise SystemExit("Missing assets/gfx or romfs/gfx")
    source = sorted(p.relative_to(ORIGINAL) for p in ORIGINAL.rglob("*")
                    if p.is_file() and p.suffix.casefold() == ".png")
    if not source:
        raise SystemExit("No original PNG artwork found")
    names: dict[Path, Path] = {}
    missing: list[str] = []
    for original in source:
        destination = mapped_name(original)
        previous = names.setdefault(destination, original)
        if previous != original:
            raise SystemExit(
                f"Two original images resolve to {destination}: {previous}, {original}")
        target = TEXTURES / destination
        if not target.is_file() or target.stat().st_size == 0:
            missing.append(str(original))
    if missing:
        raise SystemExit(
            f"{len(missing)} original PNGs have no nonempty preconverted .t3x: "
            + ", ".join(missing[:25]))
    if not ALL_IMAGES.is_file():
        raise SystemExit("Missing complete original image runtime catalog")
    full_catalog = ALL_IMAGES.read_text(encoding="utf-8")
    match = re.search(r"kOriginalImageCount=(\d+);", full_catalog)
    if not match or int(match.group(1)) != len(source):
        raise SystemExit("Original runtime image catalog count mismatch")
    for original in source:
        row = "  {" + json.dumps(original.as_posix(), ensure_ascii=False) + "," + json.dumps(mapped_name(original).as_posix(), ensure_ascii=False) + "},"
        if row not in full_catalog:
            raise SystemExit(f"Original image runtime mapping missing: {original}")
    if not SPRITES.is_file() or not ITEMS.is_file():
        raise SystemExit("Missing generated Construct sprite or item mappings")
    sprites = SPRITES.read_text(encoding="utf-8")
    items = ITEMS.read_text(encoding="utf-8")
    frame_rows = FRAME.findall(sprites)
    # The supplied mp_itemtable.js has THREE separate catalogs. Count only
    # the primary single-player item-to-sprite array here; matching the same
    # two-column regex against all generated MP player-drop/variant arrays
    # incorrectly doubled the count and caused a CI failure.
    primary_start = items.find("kItemSpriteDefinitions[]")
    primary_end = items.find("};", primary_start)
    if primary_start < 0 or primary_end < 0:
        raise SystemExit("Missing primary original item sprite array")
    item_rows = ITEM.findall(items[primary_start:primary_end])
    for array_name,count_name in (
        ("kMpPlayerDropSpriteDefinitions[]","kMpPlayerDropSpriteCount"),
        ("kMpSpriteVariants[]","kMpSpriteVariantCount"),
    ):
        start=items.find(array_name)
        end=items.find("};",start)
        count=re.search(rf"{count_name}\s*=\s*(\d+);",items)
        if start<0 or end<0 or not count or len(ITEM.findall(items[start:end]))!=int(count.group(1)):
            raise SystemExit(f"Generated multiplayer source mapping mismatch: {array_name}")
    if not frame_rows or not item_rows:
        raise SystemExit("Empty generated original sprite or item atlas mapping")
    declared_frames = re.search(r"kSpriteFrameCount=(\d+);", sprites)
    declared_items = re.search(r"kItemSpriteDefinitionCount\s*=\s*(\d+);", items)
    if not declared_frames or int(declared_frames.group(1)) != len(frame_rows):
        raise SystemExit("Generated frame record count does not match sprite header")
    if not declared_items or int(declared_items.group(1)) != len(item_rows):
        raise SystemExit("Generated item record count does not match item header")
    object_frames: set[int] = set()
    atlas_files: set[str] = set()
    atlas_missing: set[str] = set()
    for object_id, animation, frame, index, atlas in frame_rows:
        object_frames.add(int(object_id))
        # The catalog is a 16-frame Tex3DS atlas, indexed by its own mapping.
        if int(index) >= 16:
            raise SystemExit(f"Invalid frame index {index} for {atlas}")
        atlas_files.add(atlas)
        path = TEXTURES / (atlas + ".t3x")
        if not path.is_file() or path.stat().st_size == 0:
            atlas_missing.add(atlas)
    if atlas_missing:
        raise SystemExit("Missing original animation atlas pages: "
                         + ", ".join(sorted(atlas_missing)[:30]))
    if 679 not in object_frames:
        raise SystemExit("Original source player sprite object 679 is missing")
    missing_items = sorted((int(item), int(obj))
                           for item, obj in item_rows if int(obj) not in object_frames)
    if missing_items:
        raise SystemExit(f"Original item PNG animations missing: {missing_items[:25]}")
    print(f"Verified {len(source)} / {len(source)} original PNGs have 3DS textures")
    print(f"Verified {len(frame_rows)} source animation frames across "
          f"{len(atlas_files)} actual sprite atlases")
    print(f"Verified player object 679 and {len(item_rows)} original item icons")
    print("NOTE: Asset packaging is complete; gameplay-specific image use "
          "must still be tested in the renderer and event systems.")


if __name__ == "__main__":
    check()
