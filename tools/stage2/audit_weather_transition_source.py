#!/usr/bin/env python3
"""Trace original Construct rain/night phase *writes*, not assumed timers.

Every returned row includes the source ACE UID, globals assigned, and all
ancestor conditions so the 3DS runtime can only port verified phase changes.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "data/stage2/original_weather_transitions.json"
RELEVANT = {"Day_mode", "Rain", "Day_count", "Day_time", "Night_time",
            "Time_of_day", "Weather", "Rain_timer", "Day_timer", "Night_timer",
            "Day_length", "Day_tick", "Timer_Hours", "Timer_Minutes",
            "Timer_Seconds", "rains_level", "Timer_Days", "Timer_Hour",
            "Timer_ALL", "StatTimeMultiplier"}

def walk(events, ancestry=()):
    for event in events if isinstance(events, list) else ():
        if not isinstance(event,list) or len(event) < 7 or event[0] != 0:
            continue
        yield event, ancestry
        yield from walk(event[7] if len(event) > 7 else (), (*ancestry, event))

def main():
    data=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
                           SOURCE.read_text(encoding="utf-8-sig")))
    rows=[]
    for sheet in data["project"][6]:
        if not isinstance(sheet,list) or len(sheet)<2:continue
        for event,ancestry in walk(sheet[1]):
            for action in event[6]:
                if not isinstance(action,list) or len(action)<6:
                    continue
                params=action[5]
                if not isinstance(params,list) or not params:
                    continue
                first=params[0]
                if not (isinstance(first,list) and len(first)==2 and
                        first[0]==11 and first[1] in RELEVANT):
                    continue
                row={"sheet":sheet[0],"uid":event[4],
                     "parent_uids":[a[4] for a in ancestry],
                     "target":first[1],"action":action,
                     "conditions":event[5],
                     "parent_conditions":[a[5] for a in ancestry[-4:]]}
                rows.append(row)
                print("SOURCE_WEATHER_TRANSITION",json.dumps(row,
                      separators=(",",":"),ensure_ascii=False)[:3700])
    if not rows:
        raise SystemExit("No original source weather transition writes")
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text(json.dumps(rows,separators=(",",":"),ensure_ascii=False),
                      encoding="utf-8")
    print("SOURCE_WEATHER_TRANSITION_TOTAL",len(rows))
if __name__=="__main__":main()
