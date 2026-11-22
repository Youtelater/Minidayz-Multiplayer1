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
OUT = ROOT / "romfs/gfx"
HEADER = ROOT / "include/minidayz/generated/sprite_catalog.hpp"
# Small batches avoid Tex3DS atlas solver failures on crowded source sheets.
MAX_FRAMES_PER_ATLAS = 16

def ident(path: str) -> str:
    # Tex3DS output stems must be ASCII, but different Unicode source names
    # can collapse to the SAME sequence of underscores (e.g. multiple
    # original Cyrillic animation sheets). Keep a stable digest whenever
    # normalization would lose information, preventing silent overwrites.
    stem = Path(path.replace("\\\\", "/")).stem
    safe = re.sub(r"[^A-Za-z0-9_]", "_", stem)
    if safe != stem:
        safe += "_" + hashlib.sha256(stem.encode("utf-8")).hexdigest()[:10]
    return safe

def cf(v) -> str:
    value = f"{float(v):.9g}"
    if "." not in value and "e" not in value.lower():
        value += ".0"
    return value + "f"

def original_player_outline(image):
    """Root c2runtime.js outline effect, width=1, RGB=(255,255,255).
    At the source pixel-art nearest sampling grid, the eight samples at
    +/-1 and +/-0.7071 resolve to the eight adjacent pixels. Preserve the
    frame rectangle and alpha, including edge clamp. Convert shader's
    premultiplied result to straight RGBA for Tex3DS.
    """
    from PIL import Image
    image = image.convert("RGBA")
    w, h = image.size
    px = image.load()
    out = Image.new("RGBA", image.size)
    dst = out.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            maximum = max(px[min(w-1, max(0, x+dx)), min(h-1, max(0, y+dy))][3]
                          for dx, dy in ((-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)))
            ina = (maximum-a)/255.0
            # The original shader writes ina + front.a*(1-ina).
            alpha = a/255.0
            outa = ina + alpha*(1.0-ina)
            if outa <= 0.0:
                dst[x,y] = (0,0,0,0)
                continue
            def channel(v):
                return max(0,min(255,round((ina + v/255.0*alpha*(1-ina))/outa*255)))
            dst[x,y] = (channel(r),channel(g),channel(b),max(0,min(255,round(outa*255))))
    return out

def main() -> int:
    if not SOURCE.is_file():
        raise SystemExit("Missing parsed Construct source; run reassemble_snapshot.py and extract_construct.py first")
    script_digest = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    source_text = SOURCE.read_text(encoding="utf-8-sig")
    source_text = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", source_text)
    project = json.loads(source_text)
    objects = project["project"][3]
    outline_family = next((f for f in project["project"][4] if f[0] == 1064), None)
    if not outline_family:
        raise SystemExit("Original player Outline family 1064 missing")
    outline_objects = set(outline_family[1:])
    outline_sheets = set()
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
                if oi in outline_objects:
                    outline_sheets.add(source_path)
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
    # Never overwrite one original Construct image atlas with another.
    atlas_owners: dict[str, str] = {}
    for source_path in sheets:
        stem = ident(source_path)
        previous = atlas_owners.setdefault(stem, source_path)
        if previous != source_path:
            raise SystemExit(
                f"Original sprite atlas name collision: {source_path} and "
                f"{previous} both map to {stem}")
    OUT.mkdir(parents=True, exist_ok=True)
    # Atlas IDs changed for lossy Unicode filename normalization. Evict
    # only stale frame-atlas outputs owned by this generator (identified
    # by their .sha256 stamp), never original preconverted PNG textures.
    expected_atlases = {
        f"{ident(source_path)}_p{page:03d}"
        for source_path, frames in sheets.items()
        for page in range((len(frames) + MAX_FRAMES_PER_ATLAS - 1)
                          // MAX_FRAMES_PER_ATLAS)
    }
    for stamp in OUT.glob("*_p???.sha256"):
        atlas_stem = stamp.name[:-len(".sha256")]
        if atlas_stem in expected_atlases:
            continue
        old_texture = OUT / (atlas_stem + ".t3x")
        if old_texture.is_file():
            old_texture.unlink()
        stamp.unlink()
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
            # Stage 1 artwork is sourced exclusively from assets/gfx.
            # Never fall back to the duplicate images/ directory.
            # Source JS may spell its asset path as images/foo.png.
            # Those are duplicate exports, not the canonical art directory.
            # Resolve only against assets/gfx, even when the JS path starts
            # with images/, to prevent accidental use of duplicate artwork.
            normalized = source_path.replace("\\\\", "/").replace("\\", "/")
            if normalized.startswith("assets/gfx/"):
                normalized = normalized[len("assets/gfx/"):]
            elif normalized.startswith("images/"):
                normalized = normalized[len("images/"):]
            exact_candidates = [ASSETS / normalized]
            exact_candidates = [p for p in exact_candidates if p.is_file() and ASSETS.resolve() in p.resolve().parents]
            if not exact_candidates:
                exact_candidates = [p for p in ASSETS.rglob(basename) if p.is_file()]

            # Multiple paths are acceptable only when their bytes are identical.
            unique = OrderedDict()
            for candidate in exact_candidates:
                digest = hashlib.sha256(candidate.read_bytes()).hexdigest()
                unique.setdefault(digest, candidate)
            if not unique:
                raise SystemExit(
                    "Missing original sprite sheet in assets/gfx: " + basename
                    + "; source reference=" + source_path
                )
            if len(unique) != 1:
                raise SystemExit(
                    "Ambiguous original sprite sheet in assets/gfx: " + basename
                    + "; source reference=" + source_path
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
                outline_entries = []
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
                            # Preserve source pixel-art texels when a frame exceeds
                            # the 3DS 1024px texture limit. LANCZOS creates
                            # colors absent from the original MiniDAYZ art.
                            frame_image = frame_image.resize(packed_size, Image.Resampling.NEAREST)
                        frame_image.save(crop, format="PNG")
                        entries.append(crop.name)
                        if source_path in outline_sheets:
                            outline_crop = page_dir / f"outline_{index:03d}.png"
                            original_player_outline(frame_image).save(outline_crop, format="PNG")
                            outline_entries.append(outline_crop.name)
                manifest = page_dir / "atlas.t3s"
                manifest.write_text("--atlas -f rgba -z auto\n" + "\n".join(entries) + "\n", encoding="utf-8")
                destination = OUT / f"{page_stem}.t3x"
                stamp = OUT / f"{page_stem}.sha256"
                fingerprint = hashlib.sha256(
                    (script_digest + hashlib.sha256(source.read_bytes()).hexdigest()
                     + json.dumps(page_items, separators=(",", ":"))).encode("utf-8")
                ).hexdigest()
                # The derived effect atlas mirrors the exact original page
                # order: runtime selects the same frame index and hotspot.
                if outline_entries:
                    effect_manifest = page_dir / "outline.t3s"
                    effect_manifest.write_text("--atlas -f rgba -z auto\n" + "\n".join(outline_entries) + "\n", encoding="utf-8")
                    effect_dest = OUT / f"{page_stem}_outline.t3x"
                    effect_stamp = OUT / f"{page_stem}_outline.sha256"
                    if not (effect_dest.is_file() and effect_stamp.is_file() and effect_stamp.read_text(encoding="ascii").strip() == fingerprint):
                        result = subprocess.run(["tex3ds", "-i", str(effect_manifest), "-o", str(effect_dest)], cwd=page_dir, check=False)
                        if result.returncode or not effect_dest.is_file():
                            raise SystemExit(f"Tex3DS failed original Outline atlas {page_stem}")
                        effect_stamp.write_text(fingerprint+"\n",encoding="ascii")
                if destination.is_file() and stamp.is_file() and stamp.read_text(encoding="ascii").strip() == fingerprint:
                    print(f"{source.name} page {page}: unchanged; reusing cached {destination.name}")
                    continue
                result = subprocess.run(["tex3ds", "-i", str(manifest), "-o", str(destination)], cwd=page_dir, check=False)
                if result.returncode or not destination.is_file():
                    raise SystemExit(f"tex3ds failed building source sprite atlas {source.name}, page {page}")
                stamp.write_text(fingerprint + "\n", encoding="ascii")
                if result.returncode or not destination.is_file():
                    raise SystemExit(f"tex3ds failed building source sprite atlas {source.name}, page {page}")
                print(f"{source.name} page {page}: {len(page_items)} exact source frames -> romfs/gfx/{destination.name}")
    print(f"Generated {len(records)} frame references across {len(sheets)} original sheets.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
