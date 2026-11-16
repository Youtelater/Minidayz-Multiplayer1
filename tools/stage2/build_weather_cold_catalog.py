#!/usr/bin/env python3
"""Extract the original difficulty-specific Day/Night/Rain cold constants.

Only exact literal Construct global writes are translated. Weather cadence,
rain occurrence and cold damage remain separate source event mechanics.
"""
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "include/minidayz/generated/weather_cold_catalog.hpp"
SOURCE_EVENTS = {
    "Novice": 5179335140440188,
    "Regular": 931046879689234,
    "Veteran": 4240905673640528,
    "Legend": 471027401658400,
}
VARIABLES = ("Day_cold", "Night_cold", "Rain_cold")

def walk(events):
    for event in events if isinstance(events, list) else ():
        if not isinstance(event, list) or len(event) < 7 or event[0] != 0:
            continue
        yield event
        if len(event) > 7:
            yield from walk(event[7])

def main():
    raw = SOURCE.read_text(encoding="utf-8-sig")
    raw = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", raw)
    project = json.loads(raw)
    game = next((sheet for sheet in project["project"][6]
                 if isinstance(sheet, list) and len(sheet) >= 2
                 and sheet[0] == "Game_events"), None)
    if game is None:
        raise SystemExit("Original Game_events missing")
    by_uid = {event[4]: event for event in walk(game[1])}
    rows = []
    for difficulty, uid in SOURCE_EVENTS.items():
        event = by_uid.get(uid)
        if event is None:
            raise SystemExit(f"Original {difficulty} weather event missing: {uid}")
        condition = [-1, 46, None, 0, False, False, False]
        if (len(event[5]) != 1 or event[5][0][:7] != condition or
                event[5][0][9] != [[11, "Difficulty"], [8, 0], [7, [2, difficulty]]]):
            raise SystemExit(f"Original weather difficulty condition changed: {difficulty}")
        values = {}
        for action in event[6]:
            if not isinstance(action, list) or len(action) < 6:
                continue
            if action[0:2] != [-1, 41]:
                continue
            params = action[5]
            if (len(params) != 2 or not isinstance(params[0], list) or
                params[0][0] != 11 or params[0][1] not in VARIABLES):
                continue
            value = params[1]
            if (not isinstance(value, list) or len(value) != 2 or value[0] != 7
                or not isinstance(value[1], list) or len(value[1]) != 2
                or value[1][0] != 0 or not isinstance(value[1][1], int)):
                raise SystemExit(f"Nonliteral source weather value: {difficulty}")
            if params[0][1] in values:
                raise SystemExit(f"Duplicate source weather value: {difficulty}")
            values[params[0][1]] = value[1][1]
        if set(values) != set(VARIABLES):
            raise SystemExit(f"Missing source weather values: {difficulty} {values}")
        rows.append((difficulty, uid, values))
    lines = ["#pragma once", "#include <cstddef>", "#include <cstdint>",
             "namespace minidayz::items {",
             "struct SourceWeatherCold { float day, night, rain; std::uint64_t event_uid; };",
             "inline constexpr std::size_t kSourceWeatherColdCount = 4;",
             "inline constexpr SourceWeatherCold kSourceWeatherCold[] = {"]
    for difficulty, uid, values in rows:
        lines.append(f"    {{{values['Day_cold']}.0f, "
                     f"{values['Night_cold']}.0f, {values['Rain_cold']}.0f, "
                     f"{uid}ULL}}, // {difficulty}")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print("Exact original difficulty weather cold values:",
          [(d, v) for d, _, v in rows])

if __name__ == "__main__":
    main()
