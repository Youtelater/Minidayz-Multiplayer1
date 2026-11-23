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
 "door audio fired only on transition": "source_apply_auto_door_intention(" in scene and "play_source_door_effect(effect);" in scene,
 "source 0.01s wait is queued": "source_door_waits_[slot]={event,p->runtime_uid,0.01f};" in scene,
 "original 348 open Destroy before wait": scene.index("set_linked_door_solid_present(index,false);")<scene.index("source_door_waits_[slot]={event,p->runtime_uid,0.01f};"),
 "source 348 collider restored if pending event fails": "set_linked_door_solid_present(" in scene and "door->source_frame==0u" in scene,
 "wait clock advances before next interaction": game_update.index("advance_source_door_waits(dt);")<game_update.index("update_source_automatic_doors();"),
 "pending source transitions block repeated scheduling": "source_door_waits_[slot].effect!=" in scene,
 "pending state clears when regenerating": "source_door_waits_.clear();" in scene,
 "original source event applied after wait": "source_apply_auto_door_intention(" in scene,
 "sound follows successful frame application": "play_source_door_effect(effect);" in scene,
 "mirrored source door point": "door.scale_x<0.0f?-1.0f:1.0f" in door,
 "mirrored roof source point": "roof.scale_x<0.0f?-1.0f:1.0f" in roof,
 "original roof every 0.1 seconds": "source_roof_elapsed_<0.1f" in scene and "source_roof_elapsed_,0.1f" in scene,
 "roof depth and wall-zone use original frame data": all(s in scene+roof for s in ("source_house_roof_decision(","source_roof_depth_world_y(","source_player_overlaps_world_sprite(","set_placement_layer(")),
 "original reciprocal house picks": all(s in scene for s in ("6583602733022543","4153213040695326","7058744064425045","reverse_pick_matches=picked==index","reverse_pick_matches &&")),
 "no unrelated wall fading nearby roof": "if(overlap && base)" in scene and "if(overlap && player_in_wall){" in scene and
  "sheltered=sheltered || (overlap && player_in_wall);" in scene and
  "if(overlap && (!player_in_wall || touching_wall))" in scene and
  "source_picked_house_foundation_overlaps_wall(" in scene,
 "original wall and door overlap picks": all(s in scene for s in ("source_picked_house_foundation_overlaps_wall(","source_house_sprites_overlap(*door,*roof)","source_house_sprites_overlap(*door,*wall)")),
 "original Tint and player Outline states": all(s in scene for s in ("source_foundation_tint_[base_index]","source_player_outline_=decision.foundation_top","source_family_contains(1064,source_skin_object)")),
 "family 1041 has separate half-second event": "source_interior_elapsed_>=0.5f" in scene,
 "source roof reset on new generation": "source_roof_background_layer_=0xffff;" in scene,
 "mirrored solid building collision": "if(scale_x<0.0f)local_x=-local_x;" in collision,
 "live building collision orientation": "placement->angle,x,y,placement->scale_x" in scene,
 "live door original selected cue": "case AutomaticDoorEffect::open_wood:" in scene and "case AutomaticDoorEffect::close_metal:" in scene,
}
for k,v in checks.items():
 print(("PASS" if v else "FAIL"),k)
assert all(checks.values()), "Original-world building door/roof runtime dispatch contract broke"
print("Building audio + collision + movement contract OK; emulator parity still requires gameplay testing")
