#!/usr/bin/env python3
"""Prevent live world doorway/roof regressions without claiming emulator parity."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
scene=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
door=(ROOT/"include/minidayz/source_door_auto.hpp").read_text(encoding="utf-8")
roof=(ROOT/"include/minidayz/source_roof_interaction.hpp").read_text(encoding="utf-8")
collision=(ROOT/"include/minidayz/terrain_source_collision.hpp").read_text(encoding="utf-8")
audio=(ROOT/"source/stage2_audio.cpp").read_text(encoding="utf-8")
start=scene.index("const float old_x=player_.state().x")
end=scene.index("update_source_house_roofs(dt);",start)
game_update=scene[start:end]
checks={
 "door pre-movement evaluation": game_update.count("update_source_automatic_doors();")==2,
 "door state checked before collision": game_update.index("update_source_automatic_doors();")<game_update.index("player_.update_movement("),
 "door state checked after collision": game_update.rindex("update_source_automatic_doors();")>game_update.index("player_.update_movement("),
 "door flags cleared before first evaluation": game_update.index("std::fill(source_door_changed_this_tick_.begin()")<game_update.index("update_source_automatic_doors();"),
 "door transition guard on each update": "source_door_changed_this_tick_[slot])continue;" in scene,
 "door transition recorded on state change": "source_door_changed_this_tick_[slot]=1u;" in scene,
 "door transition vector rebuilt with source placement index": "source_door_changed_this_tick_.assign(source_door_indices_.size(),0u)" in scene,
 "original door point and source UIDs": all(s in door for s in ("source_door_in_y","6061166023614368ULL","243854400993751ULL")),
 "no placeholder door sounds": all(s in audio for s in ("open_door.wav","close_door.wav","open_door_metal.wav","close_door_metal.wav")),
 "door audio fired only on transition": "if(!apply_verified_door_frame_at_index(" in door,
 "mirrored source door point": "door.scale_x<0.0f?-1.0f:1.0f" in door,
 "mirrored roof source point": "roof.scale_x<0.0f?-1.0f:1.0f" in roof,
 "mirrored solid building collision": "if(scale_x<0.0f)local_x=-local_x;" in collision,
 "live building collision orientation": "placement->angle,x,y,placement->scale_x" in scene,
 "live door original selected cue": "case AutomaticDoorEffect::open_wood:" in scene and "case AutomaticDoorEffect::close_metal:" in scene,
}
for k,v in checks.items():
 print(("PASS" if v else "FAIL"),k)
assert all(checks.values()), "Original-world building door/roof runtime dispatch contract broke"
print("Building audio + collision + movement contract OK; emulator parity still requires gameplay testing")
