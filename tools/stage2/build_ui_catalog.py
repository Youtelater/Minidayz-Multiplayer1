#!/usr/bin/env python3
"""Generate native UI strings from the original Mini DAYZ English UI XML."""
from __future__ import annotations

import json
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "l_eng_ui.xml"
BUNDLED_SOURCE = ROOT / "Minidayz-Multiplayer-main" / "l_eng_ui.xml"
OUTPUT = ROOT / "include" / "minidayz" / "generated" / "ui_catalog.hpp"


def main() -> None:
    if not SOURCE.is_file() or not BUNDLED_SOURCE.is_file():
        raise SystemExit("Original root or bundled English UI localization is missing")
    normalize = lambda value: value.replace("\r\n", "\n").replace("\r", "\n")
    if normalize(SOURCE.read_text(encoding="utf-8")) != normalize(
            BUNDLED_SOURCE.read_text(encoding="utf-8")):
        raise SystemExit("Root and bundled original UI localization files differ")
    root = ET.parse(SOURCE).getroot()
    rows: list[tuple[int, str]] = []
    for entry in root.iter("text"):
        raw_id = entry.findtext("id")
        value = entry.findtext("name")
        if raw_id is None or value is None:
            raise SystemExit("UI text entry is missing its id or name")
        text_id = int(raw_id.strip())
        if text_id < 0 or text_id > 65535:
            raise SystemExit(f"Invalid original UI text id: {text_id}")
        rows.append((text_id, value.strip()))
    if not rows:
        raise SystemExit("Original UI localization contains no text entries")
    lines = [
        "#pragma once",
        "#include <cstddef>",
        "#include <cstdint>",
        "namespace minidayz::ui {",
        "struct SourceUiText { std::uint16_t id; const char* value; };",
        f"inline constexpr std::size_t kSourceUiTextCount = {len(rows)};",
        "inline constexpr SourceUiText kSourceUiTexts[] = {",
    ]
    for text_id, value in rows:
        lines.append("    {" + str(text_id) + ", " + json.dumps(value, ensure_ascii=False) + "},")
    lines += [
        "};",
        "inline constexpr const char* source_ui_text(std::uint16_t id, std::size_t occurrence = 0) noexcept {",
        "    for (const auto& entry : kSourceUiTexts) {",
        "        if (entry.id != id) continue;",
        "        if (occurrence == 0) return entry.value;",
        "        --occurrence;",
        "    }",
        '    return "";',
        "}",
        "} // namespace minidayz::ui",
        "",
    ]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} original UI strings from {SOURCE.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
