#!/usr/bin/env python3
"""Extract Construct Tilemap Solid collision polygons from the ROOT MiniDAYZ game.

This is not a visual approximation. Project object[13] stores normalized
per-tile collision vertices; missing polygons use Construct's full-cell
fallback. Preserve the original shape coordinates and tile IDs so sprite
and collision use the same source record, including flipped tile cells.
"""
from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data.deobfuscated.js"
OUTPUT = ROOT / "include/minidayz/generated/source_solid_tile_polygons.hpp"
ORIGINAL_SOLID_TILEMAPS = (208, 211)


def cpp_float(value: float) -> str:
    result = format(float(value), ".9g")
    return (result if "." in result or "e" in result.lower()
            else result + ".0") + "f"


def render() -> str:
    original = SOURCE.read_text(encoding="utf-8-sig")
    original = re.sub(r"(?<![A-Za-z0-9_.])-\\s+(?=\\d)", "-", original)
    project = json.loads(original)["project"]
    objects = project[3]
    rows = []
    coords = []
    for object_id in ORIGINAL_SOLID_TILEMAPS:
        obj = objects[object_id]
        if obj[1] != 16 or not any(
                isinstance(b, list) and b[:2] == ["Solid", 28]
                for b in obj[8]):
            raise SystemExit(f"Original tilemap {object_id} lost its Solid behavior")
        polygons = obj[13]
        if not isinstance(polygons, list) or not polygons:
            raise SystemExit(f"Original tilemap {object_id} lacks source polygons")
        for tile_id, polygon in enumerate(polygons):
            if polygon is None:
                rows.append((object_id, tile_id, 0, 0))
                continue
            if (not isinstance(polygon, list) or len(polygon) < 6 or
                    len(polygon) % 2 != 0):
                raise SystemExit(f"Invalid original tile polygon {object_id}:{tile_id}")
            if any(not isinstance(x, (int, float)) or not math.isfinite(x)
                   or x < 0.0 or x > 1.0 for x in polygon):
                raise SystemExit(f"Invalid source polygon coordinate {object_id}:{tile_id}")
            first = len(coords)
            coords.extend(zip(polygon[::2], polygon[1::2]))
            rows.append((object_id, tile_id, first, len(polygon)//2))
    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::terrain {",
        "// EXACT original Construct object[13] Tilemap Solid shape records.",
        "struct OriginalSolidTileVertex { float x,y; };",
        "struct OriginalSolidTilePolygon {",
        " std::uint16_t object,tile;std::uint32_t first;std::uint16_t count; };",
        f"inline constexpr std::size_t kOriginalSolidTilePolygonCount={len(rows)};",
        f"inline constexpr std::size_t kOriginalSolidTileVertexCount={len(coords)};",
        "inline constexpr OriginalSolidTileVertex kOriginalSolidTileVertices[]={",
    ]
    lines.extend(
        f" {{{cpp_float(x)},{cpp_float(y)}}}," for x,y in coords
    )
    lines.extend(["};", "inline constexpr OriginalSolidTilePolygon kOriginalSolidTilePolygons[]={"])
    lines.extend(f" {{{o},{i},{first},{count}}}," for o,i,first,count in rows)
    lines.extend(["};", "} // namespace minidayz::terrain", ""])
    return "\n".join(lines)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    opt = ap.parse_args()
    text = render()
    if opt.check:
        if not OUTPUT.is_file() or OUTPUT.read_text(encoding="utf-8") != text:
            raise SystemExit("Root original Tilemap Solid polygon header out of sync")
        print("PASS original Tilemap 208/211 Solid polygon mapping")
    else:
        OUTPUT.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT.write_text(text, encoding="utf-8")
        print(f"Generated source tilemap polygons: {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
