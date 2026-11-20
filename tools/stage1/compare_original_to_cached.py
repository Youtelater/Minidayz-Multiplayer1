#!/usr/bin/env python3
"""Compare root authoritative deobfuscated source and cached Construct snapshot."""
import json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def load(data):
    return json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",data.decode("utf-8-sig")))["project"]
a=load((ROOT/"data.deobfuscated.js").read_bytes())
b=load(b"".join(p.read_bytes() for p in sorted((ROOT/"data/stage1/source_snapshot").glob("part_*.jsonpart"))))
def entries(p):
    out={}
    def visit(arr,sheet,parents=()):
        for index,e in enumerate(arr):
            if isinstance(e,list) and e and e[0]==0:
                out[(sheet,parents+(index,))]={"uid":e[4],"head":e[:7],"children":[x[4] for x in e[7] if isinstance(x,list) and len(x)>4 and x[0]==0]}
                visit(e[7],sheet,parents+(index,))
    for s in p[6]:visit(s[1],s[0])
    return out
ea=entries(a);eb=entries(b)
only_a=sorted(set(ea)-set(eb));only_b=sorted(set(eb)-set(ea))
changed_event=[path for path in ea.keys()&eb.keys() if ea[path]!=eb[path]]
section=[{"index":i,"same":a[i]==b[i],"type":type(a[i]).__name__,"old_type":type(b[i]).__name__,
          "length":len(a[i]) if hasattr(a[i],"__len__") else None,
          "cached_length":len(b[i]) if hasattr(b[i],"__len__") else None}
         for i in range(min(len(a),len(b)))]
objdiff=[i for i,(x,y) in enumerate(zip(a[3],b[3])) if x!=y]
layoutdiff=[i for i,(x,y) in enumerate(zip(a[5],b[5])) if x!=y]
audiodiff=[i for i,(x,y) in enumerate(zip(a[7],b[7])) if x!=y]
output={"source_sections":len(a),"cached_sections":len(b),"section_differences":section,
        "source_event_count":len(ea),"snapshot_event_count":len(eb),
        "only_original_uids":only_a[:100],"only_cached_uids":only_b[:100],
        "differing_event_count":len(changed_event),"differing_event_paths_sample":[str(x) for x in changed_event[:100]],
        "differing_object_count":len(objdiff),"differing_object_ids_sample":objdiff[:100],
        "differing_layout_count":len(layoutdiff),"differing_layout_ids":layoutdiff,
        "differing_audio_resource_count":len(audiodiff),
        "differing_audio_resource_ids_sample":audiodiff[:100]}
out=ROOT/"audit-output/world"
out.mkdir(parents=True,exist_ok=True)
(out/"source_vs_snapshot_diff.json").write_text(json.dumps(output,ensure_ascii=False,indent=2)+"\n")
print(json.dumps({k:v for k,v in output.items() if k in ("source_event_count","snapshot_event_count",
    "differing_event_count","differing_object_count","differing_layout_count","differing_audio_resource_count",
    "differing_event_paths_sample","section_differences")},ensure_ascii=False,indent=2))
