#!/usr/bin/env python3
"""Generate MP quick-pistol IDs from the ORIGINAL root mp_kit.js.

mp_players.js identifies the independent quick slot and Drop_pistol event;
mp_kit.js identifies exactly which item IDs are handguns/quick pistols.
Do not conflate all firearms with the MP pistol equipment category.
"""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/"mp_kit.js").read_text(encoding="utf-8-sig")
player=(ROOT/"mp_players.js").read_text(encoding="utf-8-sig")
source=re.sub(r"\\u([0-9a-fA-F]{4})",lambda m:chr(int(m.group(1),16)),source)
player=re.sub(r"\\u([0-9a-fA-F]{4})",lambda m:chr(int(m.group(1),16)),player)
assert '"quick":' in player and '"Drop_pistol"' in player
match=re.search(r"_xd5bd=\\[([^\\]]+)\\]",source)
assert match,"MP kit handgun array missing"
codes=re.findall(r"(\\d+)\\^(\\d+)",match.group(1))
ids=sorted({int(a)^int(b) for a,b in codes})
assert ids==[151,152,153,165,170,171,172,176,180,181,182,234,235,236,239,244],ids
assert '"key":"pistol"' in source and '"ids":_xd5bd' in source
header=ROOT/"include/minidayz/generated/mp_quick_pistol_catalog.hpp"
header.parent.mkdir(parents=True,exist_ok=True)
header.write_text(
    "#pragma once\\n#include <cstdint>\\nnamespace minidayz::items {\\n"
    "// Generated from root mp_kit.js handgun category; mp_players.js quick slot.\\n"
    "inline constexpr std::uint16_t kMpQuickPistolItemIds[]={"
    +",".join(map(str,ids))+"};\\n"
    "inline constexpr bool is_mp_quick_pistol(std::uint16_t item) noexcept {\\n"
    "    for(auto id:kMpQuickPistolItemIds)if(id==item)return true;\\n"
    "    return false;\\n}\\n} // namespace minidayz::items\\n",
    encoding="utf-8")
print("PASS: original MP handgun quick slot has",len(ids),"source item IDs")
