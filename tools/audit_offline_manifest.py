#!/usr/bin/env python3
"""Verify offline.js, the original offline resource list, against canonical 3DS assets.

Do not copy the duplicate images/ directory. A manifest entry proves that
the original browser cached a resource, not that its gameplay event is ported.
The two browser codecs of a sound map to one same-named PCM WAV.
"""
import json
from pathlib import Path, PurePosixPath
from collections import Counter

ROOT=Path(__file__).resolve().parents[1]
MANIFEST=ROOT/"offline.js"
OUTPUT=ROOT/"data/offline_manifest_coverage.json"

def verify(root=ROOT):
    manifest=json.loads((root/"offline.js").read_text(encoding="utf-8-sig"))
    files=manifest["fileList"]
    assert isinstance(files,list) and files and all(isinstance(p,str) for p in files)
    assert len(files)==len(set(files)), "Duplicate original offline paths"
    rows=[]
    missing=[]
    for name in files:
        relative=PurePosixPath(name)
        assert not relative.is_absolute() and ".." not in relative.parts, name
        if relative.parts[0]=="images" and relative.suffix.lower()==".png":
            original="assets/gfx/"+name.removeprefix("images/")
            converted="romfs/gfx/"+str(PurePosixPath(name.removeprefix("images/")).with_name(
                relative.stem.replace("-","_").replace(".","_")+".t3x"))
            kind="image"
        elif relative.parts[0]=="media" and relative.suffix.lower() in (".m4a",".ogg"):
            original=name
            converted="romfs/wav/"+str(PurePosixPath(name.removeprefix("media/")).with_suffix(".wav"))
            kind="audio"
        else:
            original=name
            converted=None
            kind="source_resource"
        # Native code replaces the browser cache/JS runtime, and source XMLs
        # can be compiled to generated catalogs. No claim of runtime parity
        # is made for such entries just because their source is present.
        original_present=(root/original).is_file() and (root/original).stat().st_size>0
        converted_present=(root/converted).is_file() and (root/converted).stat().st_size>0 if converted else None
        rows.append(dict(original_manifest_path=name,kind=kind,canonical_source=original,
                         source_present=original_present,native_asset=converted,native_asset_present=converted_present))
        if not original_present:
            missing.append(original)
        if converted and not converted_present:
            missing.append(converted)
    counts=Counter(row["kind"] for row in rows)
    audio={r["native_asset"] for r in rows if r["kind"]=="audio"}
    images={r["canonical_source"] for r in rows if r["kind"]=="image"}
    canonical={p.relative_to(root).as_posix() for p in (root/"assets/gfx").rglob("*")
               if p.is_file() and p.suffix.lower()==".png"}
    report=dict(version=manifest["version"],manifest_entries=len(rows),
                counts=dict(counts),distinct_native_wav_count=len(audio),
                canonical_png_count=len(canonical),additional_canonical_png_count=len(canonical-images),
                missing=sorted(set(missing)),entries=rows,
                note="Resource presence is not proof of event execution. Non-media resources require separate native source-parity checks.")
    return report

if __name__=="__main__":
    result=verify()
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print("Original offline manifest:",result["manifest_entries"],"entries,",result["counts"],
          result["distinct_native_wav_count"],"distinct WAV mappings;",
          result["additional_canonical_png_count"],"additional canonical PNGs")
    if result["missing"]:
        raise SystemExit("Missing offline source/native resources: "+", ".join(result["missing"][:40]))
    print("PASS offline.js original resource coverage (not gameplay-event parity)")
