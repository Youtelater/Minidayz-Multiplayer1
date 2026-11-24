#!/usr/bin/env python3
"""Verify the simple bunker box drop catalog against the root original."""
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]
p=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
    (ROOT/"data.deobfuscated.js").read_text(encoding="utf-8-sig")))["project"]
def walk(events):
    for e in events:
        if isinstance(e,list) and e and e[0]==0:
            yield e
            if len(e)>7:
                yield from walk(e[7])
ev=next(e for e in walk(next(s[1] for s in p[6] if s[0]=="Game_events"))
        if e[4]==389950912936355)
def offset(expr,axis):
    base=[20,796,axis,False,None]
    if expr==base:
        return 0
    assert expr[0] in (4,5) and expr[1]==base and expr[2][0]==0
    return expr[2][1]*(1 if expr[0]==4 else -1)
rows=[]; choices=[]; cats=[]
for child in ev[7]:
    cond=child[5]; acts=child[6]
    if len(cond)!=1 or (len(child)>7 and child[7]) or not acts:
        continue
    if not all(a[:2]==[189,69] and a[5][0]==[1,[2,"Spawn_drop"]] for a in acts):
        continue
    q=cond[0]
    assert q[:2]==[796,71] and q[9][:2]==[[10,0],[8,0]]
    cat=q[9][2][1][1]; first=len(rows)
    for a in acts:
        args=[x[1] for x in a[5][1][1:]]
        assert len(args)==3 or (len(args)==6 and args[3:]==[[0,-1]]*3)
        expr=args[0]
        if expr[0]==0:
            ids=[expr[1]]
        else:
            assert expr[:2]==[19,82] and all(x[0]==0 for x in expr[2])
            ids=[x[1] for x in expr[2]]
        rows.append([len(choices),len(ids),int(expr[0]==19),
                     offset(args[1],65),offset(args[2],66),a[3]])
        choices.extend(ids)
    cats.append([cat,first,len(rows)-first,child[4]])
header=(ROOT/"include/minidayz/generated/source_box_simple_drops.hpp").read_text()
def values(name):
    s=header.split(name+"[]={",1)[1].split("};",1)[0]
    return [list(map(int,re.findall(r"-?\d+",row))) for row in re.findall(r"\{([^{}]+)\}",s)]
assert rows==values("kSourceBoxDrops")
assert cats==values("kSourceBoxCategories")
assert choices==list(map(int,header.split("kSourceBoxChoices[]={",1)[1].split("};",1)[0].split(",")))
assert len(cats)==30 and len(rows)==69
print("Original lootbox simple branches:",len(cats),"drops:",len(rows))

# The normal-map nested cases must retain branch order, weighted choices,
# source offsets and exact action UIDs. Locked branches use source default 0.
native=(ROOT/"include/minidayz/source_box_simple_drops.hpp").read_text()
nested=native.split("inline bool source_box_default_nested_drops",1)[1].split("inline bool source_open_simple_box",1)[0]
for child in ev[7]:
    if len(child[5])!=1:
        continue
    cat=child[5][0][9][2][1][1] if child[5][0][:2]==[796,71] else 0
    if cat not in (51,52,53):
        continue
    block=nested.split(f"case {cat}: {{",1)[1].split("        return false;\n    }",1)[0]
    pool=child[6][0][5][1][1]
    assert pool[:2]==[19,82]
    assert "constexpr int pool[]={"+",".join(str(x[1]) for x in pool[2])+"};" in block
    expected=[]; retries=[]
    for b in child[7]:
        if not b[5]:
            assert cat==53 and b[6][0][5][0]==[1,[2,"Save_run_progress"]]
            continue
        r=b[5][0][9][2][1][1]
        acts=b[6]
        if len(b)>7 and b[7]:
            if cat==51:
                assert r==23 and b[7][1][5][0][1]==70
                acts=b[7][1][6]
            else:
                assert cat==52 and len(b[7])==1
                c=b[7][0]
                retry=c[6][0][5][1][1]
                assert c[5][0][9][2]==[7,[0,0]]
                retries.append((r,",".join(str(x[1]) for x in retry[2])))
                continue
        for a in acts:
            assert a[:2]==[189,69] and a[5][0]==[1,[2,"Spawn_drop"]]
            args=[x[1] for x in a[5][1][1:]]
            assert len(args)==3 or args[3:]==[[0,-1]]*3
            expected.append((args[0][1],offset(args[1],65),offset(args[2],66),a[3]))
    actual=[tuple(map(int,m)) for m in re.findall(r"if\(!drop\((\d+),(-?\d+),(-?\d+)\)\)return false; // (\d+)",block)]
    assert actual==expected,(cat,actual,expected)
    for r,retry in retries:
        assert re.search(rf"if\(randy=={r}\) \{{[^\n]*\n\s*constexpr int retry\[\]=\{{{retry}\}};",block)
    print("Original normal bunker box",cat,"drop leaves:",len(expected))
