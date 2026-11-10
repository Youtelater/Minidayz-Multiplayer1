#!/usr/bin/env python3
"""Build exact Construct sprite-frame atlases and a C++ lookup table.

Reads only the reassembled original Construct export and assets/gfx. Each
unique source frame rectangle is cropped to a temporary PNG, grouped by its
original source sheet, and packed into a Tex3DS sheet. Original PNGs are never
modified. Frame order in each output atlas is recorded in sprite_catalog.hpp.
"""
from __future__ import annotations
import hashlib, json, re, shutil, subprocess, sys, tempfile
from collections import OrderedDict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
ASSETS = ROOT / "assets/gfx"
OUT = ROOT / "romfs/gfx/sprites"
HEADER = ROOT / "include/minidayz/generated/sprite_catalog.hpp"
# Small batches avoid Tex3DS atlas solver failures on crowded source sheets.
MAX_FRAMES_PER_ATLAS = 16

def ident(path: str) -> str:
    stem = Path(path).stem.replace("-", "_").replace(".", "_")
    return re.sub(r"[^A-Za-z0-9_]", "_", stem)

def cf(v) -> str:
    return f"{float(v):.9g}f"

def main() -> int:
    if not SOURCE.is_file():
        raise SystemExit("Missing parsed Construct source; run reassemble_snapshot.py and extract_construct.py first")
    script_digest = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()\n    project = json.loads(SOURCE.read_text(encoding="utf-8"))
    objects = project["project"][3]
    # Preserve first-seen source-sheet and frame order from the original object
    # and animation arrays. Deduplicate only identical source rectangles.
    sheets: OrderedDict[str, OrderedDict[tuple, int]] = OrderedDict()
    records = []
    animations = []
    for oi, obj in enumerate(objects):
        for ai, anim in enumerate(obj[7] if len(obj) > 7 and isinstance(obj[7], list) else []):
            if not isinstance(anim, list) or not anim:
                continue
            frames = anim[7] if len(anim) > 7 and isinstance(anim[7], list) else []
            animations.append((oi, ai, str(anim[0]), len(frames), float(anim[3]) if len(anim)>3 else 0.0, bool(anim[2]) if len(anim)>2 else False))
            for fi, fr in enumerate(frames):
                if not isinstance(fr, list) or not fr or not isinstance(fr[0], str):
                    continue
                source_path = fr[0]
                key = (int(fr[2]), int(fr[3]), int(fr[4]), int(fr[5]))
                if key[2] <= 0 or key[3] <= 0:
                    raise SystemExit(f"Invalid source frame rectangle: object={oi} animation={anim[0]} frame={fi} rect={key}")
                by_frame = sheets.setdefault(source_path, OrderedDict())
                atlas_index = by_frame.setdefault(key, len(by_frame))
                page = atlas_index // MAX_FRAMES_PER_ATLAS
                page_image = atlas_index % MAX_FRAMES_PER_ATLAS
                scale = min(1.0, 1024.0 / max(key[2], key[3]))
                packed_w = max(1, round(key[2] * scale))
                packed_h = max(1, round(key[3] * scale))
                records.append((oi, ai, fi, source_path, page, page_image, key[2], key[3],
                                float(fr[7]) if len(fr)>7 else 0.5,
                                float(fr[8]) if len(fr)>8 else 0.5,
                                key[2] / packed_w, key[3] / packed_h))
    OUT.mkdir(parents=True, exist_ok=True)
    HEADER.parent.mkdir(parents=True, exist_ok=True)
    header = [
        "#pragma once", "#include <cstdint>", "#include <cstddef>",
        "namespace minidayz::stage1 {",
        "struct SpriteFrameDef { std::uint16_t object, animation; std::uint32_t frame, image; const char* sheet; float width,height,origin_x,origin_y,texture_scale_x,texture_scale_y; };",
        "struct SpriteAnimationDef { std::uint16_t object, animation; const char* name; std::uint32_t frame_count; float source_speed; bool loop; };",
        f"inline constexpr std::size_t kSpriteFrameCount={len(records)};",
        f"inline constexpr std::size_t kSpriteAnimationCount={len(animations)};",
        "inline constexpr SpriteFrameDef kSpriteFrames[] = {"
    ]
    for oi, ai, fi, path, page, idx, w, h, ox, oy, tx, ty in records:
        header.append(f'{{{oi},{ai},{fi},{idx},"{ident(path)}_p{page:03d}",{cf(w)},{cf(h)},{cf(ox)},{cf(oy)},{cf(tx)},{cf(ty)}}},')
    header += ["};", "inline constexpr SpriteAnimationDef kSpriteAnimations[] = {"]
    for oi, ai, name, count, speed, loop in animations:
        safe = name.replace("\\", "\\\\").replace('"', '\\"')
        header.append(f'{{{oi},{ai},"{safe}",{count},{cf(speed)},{str(loop).lower()}}},')
    header += ["};", "} // namespace minidayz::stage1", ""]
    HEADER.write_text("\n".join(header), encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="minidayz_frames_") as tmp:
        temp = Path(tmp)
        from PIL import Image
        for source_path, frame_map in sheets.items():
            # The original project stores some GUI item sheets under images/
            # while most world/UI sheets live under assets/gfx/. Resolve by exact
            # basename only; never substitute a similarly named artwork file.
            source_ref = Path(source_path)
            basename = source_ref.name
            # First honor the exact relative path recorded in the original
            # Construct frame. Only then use the known asset roots.
            exact_candidates = [
                ROOT / source_ref,
                ASSETS / source_ref,
                ROOT / "images" / source_ref,
            ]
            exact_candidates = [p for p in exact_candidates if p.is_file()]
            if not exact_candidates:
                exact_candidates = [
                    p for p in (ASSETS / basename, ROOT / "images" / basename)
                    if p.is_file()
                ]
            if not exact_candidates:
                exact_candidates = list(ASSETS.rglob(basename)) + list((ROOT / "images").rglob(basename))
            # Mirrored files are acceptable only when their bytes match.
            unique = OrderedDict()
            for candidate in exact_candidates:
                digest = hashlib.sha256(candidate.read_bytes()).hexdigest()
                unique.setdefault(digest, candidate)
            if len(unique) != 1:
                raise SystemExit(
                    "Ambiguous original sprite sheet " + basename
                    + ": source reference=" + source_path
                    + "; candidates: "
                    + ", ".join(str(path) for path in exact_candidates)
                )
            source = next(iter(unique.values()))
            stem = ident(source_path)
            frame_items = list(frame_map.keys())
            for page, first in enumerate(range(0, len(frame_items), MAX_FRAMES_PER_ATLAS)):
                page_items = frame_items[first:first+MAX_FRAMES_PER_ATLAS]
                page_stem = f"{stem}_p{page:03d}"
                page_dir = temp / page_stem
                page_dir.mkdir()
                entries = []
                with Image.open(source) as image:
                    image.load()
                    for index, (x,y,w,h) in enumerate(page_items):
                        # Construct frame rectangles can extend beyond the source sheet.
                        # Preserve the declared frame dimensions and offset by padding
                        # out-of-sheet pixels transparently; never shift or resize artwork.
                        left, top = max(0, x), max(0, y)
                        right, bottom = min(image.width, x + w), min(image.height, y + h)
                        if right <= left or bottom <= top:
                            raise SystemExit(f"Frame has no pixels inside {source.name}: {(x,y,w,h)} vs {image.size}")
                        crop = page_dir / f"frame_{index:03d}.png"
                        frame_image = Image.new("RGBA", (w, h), (0, 0, 0, 0))
                        visible = image.convert("RGBA").crop((left, top, right, bottom))
                        frame_image.paste(visible, (left - x, top - y))
                        # 3DS textures are limited to 1024x1024. Resize only
                        # this temporary extracted frame; source artwork and
                        # logical Construct dimensions remain unchanged.
                        scale = min(1.0, 1024.0 / max(w, h))
                        if scale < 1.0:
                            packed_size = (max(1, round(w * scale)), max(1, round(h * scale)))
                            frame_image = frame_image.resize(packed_size, Image.Resampling.LANCZOS)
                        frame_image.save(crop, format="PNG")
                        entries.append(crop.name)
                manifest = page_dir / "atlas.t3s"
                manifest.write_text("--atlas -f rgba -z auto\n" + "\n".join(entries) + "\n", encoding="utf-8")
                destination = OUT / f"{page_stem}.t3x"
                stamp = OUT / f"{page_stem}.sha256"
                fingerprint = hashlib.sha256(
                    (script_digest + hashlib.sha256(source.read_bytes()).hexdigest()
                     + json.dumps(page_items, separators=(",", ":"))).encode("utf-8")
                ).hexdigest()
                if destination.is_file() and stamp.is_file() and stamp.read_text(encoding="ascii").strip() == fingerprint:
                    print(f"{source.name} page {page}: unchanged; reusing cached {destination.name}")
                    continue
                result = subprocess.run(["tex3ds", "-i", str(manifest), "-o", str(destination)], cwd=page_dir, check=False)
                if result.returncode or not destination.is_file():
                    raise SystemExit(f"tex3ds failed building source sprite atlas {source.name}, page {page}")
                stamp.write_text(fingerprint + "\\n", encoding="ascii")
                if result.returncode or not destination.is_file():
                    raise SystemExit(f"tex3ds failed building source sprite atlas {source.name}, page {page}")
                print(f"{source.name} page {page}: {len(page_items)} exact source frames -> {destination.name}")
    print(f"Generated {len(records)} frame references across {len(sheets)} original sheets.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
