#!/usr/bin/env python3
"""Check weather sound playback uses the source world's winter category.

Source rain eligibility is distinct from the 3DS ambience winter selection.
The old kSourceDefaultRainsLevel==2 expression was unconditionally true.
This guard checks native call-site mapping, not full Construct audio parity.
"""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
audio=(ROOT/"source/stage2_audio.cpp").read_text(encoding="utf-8")
assert "kSourceDefaultRainsLevel==2" not in main
calls=re.findall(r"stage2_audio_\.start_rain_loop\(\s*(.*?)\s*\)",main)
assert len(calls)==3, f"expected load/unmute/weather-start sites: {calls!r}"
assert all(expr=="world_level_==4" for expr in calls), calls
assert "rain_timer_.update(rain_dt,kSourceDefaultRainsLevel,false,rain_choice)" in main
assert 'variant==1 ? "romfs:/wav/blizzard.wav"' in audio
assert '"romfs:/wav/rain_loop.wav"' in audio
assert "set_weather_sample_variant(blizzard?1:0)" in audio
print("PASS: All three rain-loop starts use winter level, not rain eligibility; original rain/blizzard WAV selection preserved")
