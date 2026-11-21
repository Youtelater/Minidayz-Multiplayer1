#!/usr/bin/env python3
"""Generate exact Construct family membership from root project[4].

World source event sheets pick families 1041 (house interior), 1048 and
1049 (roof overlays). They have *no sprite frames themselves*; their
member objects own the PNG frames, image points and collision polygons.
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
families=p[4]
pairs=[]
for family in families:
    if not isinstance(family,list) or len(family)<2:continue
    source_family=family[0]
    for obj in family[1:]:
        if not isinstance(obj,int) or not (0<=obj<len(p[3])):
            raise SystemExit(f"Invalid source family {source_family}: {obj!r}")
        pairs.append((source_family,obj))
pairs=sorted(set(pairs))
for fam in (1041,1048,1049):
    members=[oid for f,oid in pairs if f==fam]
    if not members:raise SystemExit(f"Original family {fam} has zero members")
    print(f"Root family {fam} members",len(members),members[:35])
out=ROOT/"include/minidayz/generated/source_family_members.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstddef>","#include <cstdint>",
"namespace minidayz::stage1 {",
"// Root Construct project[4] family membership, not a guessed C++ object list.",
"struct SourceFamilyMember { std::uint16_t family,object; };",
"inline constexpr SourceFamilyMember kSourceFamilyMembers[]={"]
lines += [" {"+str(f)+","+str(o)+"}," for f,o in pairs]
lines += ["};",
"inline constexpr std::size_t kSourceFamilyMemberCount=sizeof(kSourceFamilyMembers)/sizeof(kSourceFamilyMembers[0]);",
"inline bool source_family_contains(std::uint16_t family,std::uint16_t object) noexcept {",
" std::size_t first=0,last=kSourceFamilyMemberCount;",
" while(first<last){",
"  const auto mid=first+(last-first)/2;",
"  const auto& item=kSourceFamilyMembers[mid];",
"  if(item.family<family || (item.family==family&&item.object<object))first=mid+1;",
"  else last=mid;",
" }",
" return first<kSourceFamilyMemberCount &&",
"    kSourceFamilyMembers[first].family==family && kSourceFamilyMembers[first].object==object;",
"}",
"} // namespace minidayz::stage1"]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Original family member pairs",len(pairs),"families",len(families))
