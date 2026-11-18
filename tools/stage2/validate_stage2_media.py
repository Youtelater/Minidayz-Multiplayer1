#!/usr/bin/env python3
"""Validate the actual original WAVs and Tex3DS atlases consumed at runtime.

Unlike checking whether filenames appear in a manifest, this validates ROMFS
paths, the source sprite catalog and PCM format/size supported by Stage2Audio.
"""
from __future__ import annotations

import argparse
import re
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WAV = ROOT / "romfs/wav"
ATLAS = ROOT / "romfs/gfx"
SPRITES = ROOT / "include/minidayz/generated/sprite_catalog.hpp"
REQUIRED_SOUNDS = ("crafting.wav", "drop1.wav", "drop2.wav", "zippero.wav",
                   "geardrop1.wav", "geardrop2.wav")
MAX_CLIP_BYTES = 4 * 1024 * 1024


def validate_audio() -> None:
    for name in REQUIRED_SOUNDS:
        path = WAV / name
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"Stage 2 original WAV missing from ROMFS: {path}")
        try:
            with wave.open(str(path), "rb") as sound:
                channels = sound.getnchannels()
                width = sound.getsampwidth()
                rate = sound.getframerate()
                frames = sound.getnframes()
                compression = sound.getcomptype()
        except (wave.Error, EOFError, OSError) as exc:
            raise SystemExit(f"Cannot decode Stage 2 original WAV {path}: {exc}") from exc
        payload = channels * width * frames
        if (compression != "NONE" or channels not in (1, 2) or width not in (1, 2)
                or not 8000 <= rate <= 48000 or frames <= 0
                or payload > MAX_CLIP_BYTES):
            raise SystemExit(
                f"Stage 2 audio incompatible with 3DS PCM loader: {name} "
                f"channels={channels} width={width} rate={rate} frames={frames} "
                f"PCM={compression} bytes={payload}")
        print(f"Original WAV OK: {name}, {channels} channels, {width*8}-bit, {rate}Hz")


def validate_atlas() -> None:
    if not SPRITES.is_file():
        raise SystemExit("Missing generated original Construct sprite atlas catalog")
    header = SPRITES.read_text(encoding="utf-8")
    sheets = set(re.findall(r'"([^"\n]+_p\d{3})"', header))
    if len(sheets) < 100:
        raise SystemExit(f"Original sprite atlas coverage unexpectedly low: {len(sheets)}")
    missing = [name for name in sorted(sheets)
               if not (ATLAS / f"{name}.t3x").is_file()
               or (ATLAS / f"{name}.t3x").stat().st_size == 0]
    if missing:
        raise SystemExit(
            "Original Construct sprite atlases missing from ROMFS: " +
            ", ".join(missing[:15]))
    print(f"Original Construct atlases OK: {len(sheets)} ROMFS .t3x files")
    # Ground tiles are Construct Tilemaps (type 16), not sprites, so the
    # regular source animation atlas cannot cover them.
    for variant in ("original_map_base_ground.t3x",
                    "original_map_base_ground_lvl2.t3x",
                    "original_map_base_ground_lvl3.t3x"):
        atlas = ATLAS / variant
        if not atlas.is_file() or atlas.stat().st_size <= 32:
            raise SystemExit("Original map-specific terrain not packaged: " + variant)
    background = ATLAS / "original_map_base_ground.t3x"
    if not background.is_file() or background.stat().st_size < 32:
        raise SystemExit("Original-source grass background missing: gameplay would show black terrain")
    floor = ATLAS / "original_map_ground_p000.t3x"
    floor_layout = ATLAS / "original_map_ground_layout.bin"
    if not floor_layout.is_file() or floor_layout.read_bytes()[:4] != b"MDG2":
        raise SystemExit("Original Construct Map ground tile placement data missing")
    floor_header = ROOT / "include/minidayz/generated/ground_floor_source.hpp"
    ui_header = ROOT / "include/minidayz/generated/inventory_ui_art.hpp"
    if not floor.is_file() or floor.stat().st_size < 32:
        raise SystemExit("Original ground floor texture missing from 3DS ROMFS")
    if not floor_header.is_file() or "kOriginalFloorTileObject=482" not in floor_header.read_text(encoding="utf-8"):
        raise SystemExit("Original ground tilemap source metadata is absent or changed")
    if not ui_header.is_file():
        raise SystemExit("Original inventory GUI source atlas mapping missing")
    for name in ("InventoryBackground","InventoryCard","SlotBackground",
                 "EquipmentVest","EquipmentCoat","EquipmentBackpack"):
        if f"kOriginal{name}Art" not in ui_header.read_text(encoding="utf-8"):
            raise SystemExit(f"Original MiniDAYZ GUI source frame missing: {name}")
    print(f"Original Map floor and inventory GUI artwork OK: {floor.stat().st_size} T3X bytes")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-atlases", action="store_true")
    args = parser.parse_args()
    validate_audio()
    if not args.skip_atlases:
        validate_atlas()


if __name__ == "__main__":
    main()
