#!/usr/bin/env python3
"""Inventory original MiniDAYZ assets without claiming that packaging equals gameplay use.

Run after make/stage1-assets. Classifies source PNG references and WAV references
conservatively: dynamic/indirect playback is explicitly not counted as proven.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "data" / "asset_coverage.json"


def original_image_references():
    parsed = ROOT / "data/stage1/construct_project_parsed.json"
    if not parsed.is_file():
        raise SystemExit("Run stage1-assets first (missing parsed Construct data)")
    data = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
                             parsed.read_text(encoding="utf-8-sig")))
    objects = data["project"][3]
    referenced = set()
    for obj in objects:
        if not isinstance(obj, list):
            continue
        # Sprite animation frames reference the original texture sheet.
        if len(obj) > 7 and isinstance(obj[7], list):
            for animation in obj[7]:
                if not isinstance(animation, list) or len(animation) <= 7:
                    continue
                for frame in animation[7] if isinstance(animation[7], list) else []:
                    if isinstance(frame, list) and frame and isinstance(frame[0], str):
                        referenced.add(Path(frame[0].replace("\\", "/")).name.casefold())
        # Construct tilemaps and other texture-bearing objects.
        if len(obj) > 6 and isinstance(obj[6], list):
            for value in obj[6]:
                if isinstance(value, str) and value.lower().endswith(".png"):
                    referenced.add(Path(value.replace("\\", "/")).name.casefold())
    return referenced


def runtime_audio_references():
    sources = list((ROOT / "source").glob("*.cpp"))
    sources += list((ROOT / "include").rglob("*.hpp"))
    texts = [p.read_text(encoding="utf-8", errors="replace") for p in sources]
    direct = {name.casefold() for text in texts
              for name in re.findall(r'romfs:/wav/([^"\s]+\.wav)', text)}
    # Source creates footstep names at runtime from a format string. These
    # are reachable, but their playback depends on actual terrain movement.
    dynamic = set()
    if any('%s_%u.wav' in text for text in texts):
        for prefix in ("road_run", "forest_run", "sand_run", "snow_run",
                       "swamp_step", "run"):
            for index in range(1, 5):
                dynamic.add(f"{prefix}_{index}.wav")
    return direct, dynamic


def check_player_item_sprites():
    sprite_header = ROOT / "include/minidayz/generated/sprite_catalog.hpp"
    item_header = ROOT / "include/minidayz/generated/item_sprite_catalog.hpp"
    if not sprite_header.is_file() or not item_header.is_file():
        raise SystemExit("Missing generated original player or item sprite catalog")
    sprite_text = sprite_header.read_text(encoding="utf-8")
    item_text = item_header.read_text(encoding="utf-8")
    frame_objects = {int(m.group(1)) for m in re.finditer(r"\{(\d+),\d+,\d+,\d+,", sprite_text)}
    item_rows = [(int(m.group(1)), int(m.group(2)))
                 for m in re.finditer(r"\{(\d+),\s*(\d+)\},", item_text)]
    if 679 not in frame_objects:
        raise SystemExit("Original player skin object 679 has no generated 3DS sprite frame")
    if not item_rows:
        raise SystemExit("No source item PNG sprites mapped to 3DS")
    missing = sorted((item, obj) for item, obj in item_rows if obj not in frame_objects)
    if missing:
        raise SystemExit(f"Item sprites missing generated frames: {missing[:20]}")
    return len(item_rows)


def main():
    gfx = ROOT / "assets/gfx"
    wav = ROOT / "romfs/wav"
    generated = ROOT / "romfs/gfx"
    if not gfx.is_dir() or not wav.is_dir():
        raise SystemExit("Expected original assets/gfx and romfs/wav")
    mapped_item_sprites = check_player_item_sprites()
    refs = original_image_references()
    direct, dynamic = runtime_audio_references()
    images = sorted(p.relative_to(gfx).as_posix() for p in gfx.rglob("*")
                    if p.is_file() and p.suffix.lower() == ".png")
    sounds = sorted(p.relative_to(wav).as_posix() for p in wav.rglob("*")
                    if p.is_file() and p.suffix.lower() == ".wav")
    image_records = [{"file": name,
                      "source_referenced": Path(name).name.casefold() in refs}
                     for name in images]
    sound_records = [{"file": name,
                      "runtime_reference": (
                          "direct" if Path(name).name.casefold() in direct
                          else "dynamic" if Path(name).name.casefold() in dynamic
                          else "not_yet_identified")}
                     for name in sounds]
    missing_image_refs = sorted(refs - {Path(n).name.casefold() for n in images})
    missing_sound_refs = sorted((direct | dynamic) -
                                {Path(n).name.casefold() for n in sounds})
    result = {
        "note": "Source-referenced or packaged is NOT proof of being rendered/played in gameplay.",
        "graphics": {
            "player_source_sprite_validated": True,
            "mapped_item_source_sprites": mapped_item_sprites,
            "original_png_count": len(images),
            "source_referenced_count": sum(x["source_referenced"] for x in image_records),
            "generated_t3x_count": len(list(generated.rglob("*.t3x"))),
            "source_references_missing_png": missing_image_refs,
            "files": image_records,
        },
        "audio": {
            "romfs_wav_count": len(sounds),
            "direct_reference_count": sum(x["runtime_reference"] == "direct" for x in sound_records),
            "dynamic_reference_count": sum(x["runtime_reference"] == "dynamic" for x in sound_records),
            "not_yet_identified_count": sum(x["runtime_reference"] == "not_yet_identified" for x in sound_records),
            "runtime_references_missing_wav": missing_sound_refs,
            "files": sound_records,
        },
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Asset inventory: {len(images)} original PNGs, {len(sounds)} WAVs")
    print(f"Original source references: {result['graphics']['source_referenced_count']} PNGs")
    print(f"Runtime audio references: {result['audio']['direct_reference_count']} direct, "
          f"{result['audio']['dynamic_reference_count']} dynamic, "
          f"{result['audio']['not_yet_identified_count']} not yet identified")
    print("Full classification: data/asset_coverage.json")
    if missing_image_refs:
        raise SystemExit("Original Construct sprite/texture references missing from assets/gfx: " +
                         ", ".join(missing_image_refs[:15]))
    if missing_sound_refs:
        raise SystemExit("Runtime WAV references missing from romfs/wav: " +
                         ", ".join(missing_sound_refs[:15]))


if __name__ == "__main__":
    main()
