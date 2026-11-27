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

# Direct Create / variable-1 quantity assignments retain both action UIDs.
spawn=next(e for e in walk(next(s[1] for s in p[6] if s[0]=="Game_events"))
           if e[4]==5418835019314848)
objects={}
for b in walk(spawn[7]):
    if len(b[5])!=1:
        continue
    q=b[5][0]
    if q[:2]!=[189,112] or q[9][:2]!=[[0,[0,0]],[8,0]] or q[9][2][1][0]!=0:
        continue
    for a in b[6]:
        if a[:2]==[-1,94]:
            objects[a[5][0][1]]=q[9][2][1][1]
direct=[]
for child in ev[7]:
    if len(child[5])!=1 or child[5][0][:2]!=[796,71]:
        continue
    cat=child[5][0][9][2][1][1]
    if not 5<=cat<=13:
        continue
    assert len(child[6])%2==0
    for a,b in zip(child[6][::2],child[6][1::2]):
        obj=a[5][0][1]
        assert a[:2]==[-1,94] and a[5][1]==[5,[2,"items_on_ground"]]
        assert b[:2]==[obj,88] and b[5][0]==[10,1]
        direct.append([cat,obj,objects[obj],b[5][1][1][1],
                       offset(a[5][2][1],65),offset(a[5][3][1],66),a[3],b[3]])
s=native.split("kSourceBoxDirectQuantityDrops[]={",1)[1].split("};",1)[0]
assert direct==[list(map(int,re.findall(r"-?\d+",row))) for row in re.findall(r"\{([^{}]+)\}",s)]
assert len(direct)==19
print("Original direct-quantity box drops:",len(direct))

# Category 999 uses three source-ordered outcomes and six literal drop leaves.
child=next(c for c in ev[7] if len(c[5])==1 and c[5][0][:2]==[796,71]
           and c[5][0][9][2][1][1]==999)
block=nested.split("case 999: {",1)[1].split("        return false;\n    }",1)[0]
pool=child[6][0][5][1][1]
assert pool[:2]==[19,82]
assert "constexpr int pool[]={"+",".join(str(x[1]) for x in pool[2])+"};" in block
expected=[]
for b in child[7]:
    r=b[5][0][9][2][1][1]
    assert f"if(randy=={r}) {{ // {b[4]}" in block
    for a in b[6]:
        assert a[:2]==[189,69] and a[5][0]==[1,[2,"Spawn_drop"]]
        args=[x[1] for x in a[5][1][1:]]
        assert args[0][0]==0 and args[3:]==[[0,-1]]*3
        expected.append((args[0][1],offset(args[1],65),offset(args[2],66),a[3]))
assert expected==[tuple(map(int,m)) for m in re.findall(r"if\(!drop\((\d+),(-?\d+),(-?\d+)\)\)return false; // (\d+)",block)]
assert len(expected)==6
print("Original category 999 drop leaves:",len(expected))

# Ordered mixed Spawn_drop and direct Create + variable-1 branches.
mixed=[]
mixed_categories={14,15,17,18,19,20,37,38,39,40,41,42,111,112,116,1011}
for child in ev[7]:
    if len(child[5])!=1 or child[5][0][:2]!=[796,71]:
        continue
    cat=child[5][0][9][2][1][1]
    if cat not in mixed_categories:
        continue
    assert not (len(child)>7 and child[7])
    acts=iter(child[6])
    for a in acts:
        if a[:2]==[189,69]:
            assert a[5][0]==[1,[2,"Spawn_drop"]]
            args=[x[1] for x in a[5][1][1:]]
            assert args[0][0]==0 and (len(args)==3 or args[3:]==[[0,-1]]*3)
            mixed.append([cat,0,args[0][1],1,offset(args[1],65),offset(args[2],66),a[3],0])
        else:
            obj=a[5][0][1]
            assert a[:2]==[-1,94] and a[5][1]==[5,[2,"items_on_ground"]]
            if cat in (14,15,17):
                matches=[e for e in walk(spawn[7]) if len(e[5])==1
                         and e[5][0][:2]==[189,112] and len(e[6])==1
                         and e[6][0][:2]==[-1,94] and e[6][0][5][0][1]==obj]
                assert len(matches)==1 and matches[0][5][0][9][2][1][1]==objects[obj]
                mixed.append([cat,obj,objects[obj],1,offset(a[5][2][1],65),
                              offset(a[5][3][1],66),a[3],0])
                continue
            b=next(acts)
            assert b[:2]==[obj,88] and b[5][0]==[10,1] and b[5][1][1][0]==0
            mixed.append([cat,obj,objects[obj],b[5][1][1][1],
                          offset(a[5][2][1],65),offset(a[5][3][1],66),a[3],b[3]])
s=native.split("kSourceBoxMixedDrops[]={",1)[1].split("};",1)[0]
assert mixed==[list(map(int,re.findall(r"-?\d+",row))) for row in re.findall(r"\{([^{}]+)\}",s)]
assert len(mixed)==41 and {row[0] for row in mixed}==mixed_categories
print("Original mixed/direct box drop actions:",len(mixed))

# Category 456789: original 15 outcomes, default locked achievement branches.
special=next(c for c in ev[7] if len(c[5])==1 and c[5][0][:2]==[796,71]
             and c[5][0][9][2][1][1]==456789)
pool=special[6][0][5][1][1]
assert pool[:2]==[19,82]
block=native.split("inline bool source_box_456789_drops",1)[1]
assert "constexpr int pool[]={"+",".join(str(x[1]) for x in pool[2])+"};" in block
special_rows=[]
for b in special[7]:
    outcome=b[5][0][9][2][1][1]
    actions=b[6]
    if len(b)>7 and b[7]:
        assert outcome in (14,15) and len(b[7])==2 and b[7][1][5][0][1]==70
        actions=b[7][1][6]
    acts=iter(actions)
    for a in acts:
        if a[:2]==[189,69]:
            assert a[5][0]==[1,[2,"Spawn_drop"]]
            args=[x[1] for x in a[5][1][1:]]
            assert args[0][0]==0 and (len(args)==3 or args[3:]==[[0,-1]]*3)
            special_rows.append([outcome,0,args[0][1],1,offset(args[1],65),offset(args[2],66),a[3],0])
        else:
            obj=a[5][0][1]
            assert a[:2]==[-1,94] and a[5][1]==[5,[2,"items_on_ground"]]
            d=next(acts)
            assert d[:2]==[obj,88] and d[5][0]==[10,1] and d[5][1][1][0]==0
            special_rows.append([outcome,obj,objects[obj],d[5][1][1][1],
                                 offset(a[5][2][1],65),offset(a[5][3][1],66),a[3],d[3]])
s=native.split("kSourceBox456789Drops[]={",1)[1].split("};",1)[0]
assert special_rows==[list(map(int,re.findall(r"-?\d+",row))) for row in re.findall(r"\{([^{}]+)\}",s)]
assert len(special_rows)==56 and {row[0] for row in special_rows}==set(range(1,16))
print("Original normal-map category 456789 drop actions:",len(special_rows))


# Special Create/Set actions: preserve original variable indices, not stacks.
def special_box(cat):
    return next(e for e in ev[7] if len(e[5])==1 and e[5][0][:2]==[796,71] and e[5][0][9][2][1][1]==cat)
special_actions=[]; special_branches=[]; special_pools=[]; special_choices=[]
def add_special(cat,outcome,e,unlock="",retry=0):
    first=len(special_actions)
    for a in e[6]:
        if a[:2]==[-1,94]:
            assert a[5][1]==[5,[2,"items_on_ground"]]
            special_actions.append([0,a[5][0][1],0,0,offset(a[5][2][1],65),offset(a[5][3][1],66),a[3]])
        else:
            assert a[1]==88 and a[5][1][1][0]==0
            special_actions.append([1,a[0],a[5][0][1],a[5][1][1][1],0,0,a[3]])
    special_branches.append([cat,outcome,first,len(special_actions)-first,e[4],unlock,retry])
for cat in (16,18,19,20,23,29): add_special(cat,0,special_box(cat))
for cat,n in ((1020,6),(1021,22)):
    e=special_box(cat)
    assert e[6][0][5][1][1]==[19,82,[[0,i] for i in range(1,n+1)]]
    for b in e[7]:
        outcome=b[5][0][9][2][1][1]
        if len(b)>7 and b[7]:
            unlocked,locked=b[7]
            assert unlocked[5][0][9][2]==[7,[0,1]] and locked[5][0][1]==70
            retry=locked[6][0][5][1][1]
            assert retry==[19,82,[[0,i] for i in range(1,len(retry[2])+1)]]
            add_special(cat,outcome,unlocked,unlocked[5][0][9][0][1],len(retry[2]))
        else: add_special(cat,outcome,b)
for a in special_box(1022)[7][0][6]:
    assert a[:2]==[189,69] and a[5][0]==[1,[2,"Spawn_drop"]]
    args=[x[1] for x in a[5][1][1:]]
    assert args[0][:2]==[19,82] and args[3]==[0,100]
    ids=[x[1] for x in args[0][2]]
    special_pools.append([len(special_choices),len(ids),offset(args[1],65),offset(args[2],66),100,a[3]])
    special_choices.extend(ids)
special_header=(ROOT/"include/minidayz/source_special_box_actions.hpp").read_text()
def special_values(name):
    body=special_header.split(name+"[]={",1)[1].split("};",1)[0]
    return [json.loads("["+s+"]") for s in re.findall(r"\{([^{}]+)\}",body)]
assert special_values("kSourceSpecialBoxActions")==special_actions
assert special_values("kSourceSpecialBoxBranches")==special_branches
assert special_values("kSourceSpecialBoxPools")==special_pools
assert json.loads("["+special_header.split("kSourceSpecialBoxChoices[]={",1)[1].split("};",1)[0]+"]")==special_choices
assert len(special_actions)==188 and len(special_branches)==34 and len(special_pools)==6
print("Original special box branches:",len(special_branches),"actions:",len(special_actions))

# Original Sprite796 Fade configuration and trigger order.
assert p[3][796][8][1][:2]==["Fade",31]
map_layout=next(l for l in p[5] if l[0]=="Map")
box_fixture=next(i for l in map_layout[6] for i in l[14] if i[1]==796)
assert box_fixture[4][1]==[0,0,2,1,1]
assert [(a[0],a[1],a[3]) for a in ev[6]]==[(796,72,402409664698433),(796,167,283511197237349),(796,161,232138073544863)]
assert ev[6][1][2]=="Fade"
print("Original bunker lootbox Fade: wait 2s, fade-out 1s, destroy")

# Normal category 53 dispatches Save_run_progress after all outcome leaves.
normal53=next(e for e in ev[7] if e[4]==483637851273824)
last=normal53[7][-1]
assert last[4]==432922065105325 and not last[5]
assert last[6]==[[189,69,None,970390035245631,False,[[1,[2,"Save_run_progress"]],[13]]]]
assert "return category==53;" in native
save_event=next(e for e in walk(next(s[1] for s in p[6] if s[0]=="Game_events")) if e[4]==398740622982942)
assert [c[9] for c in save_event[7][0][5]]==[[[11,"levelchanging"],[8,0],[7,[0,0]]],[[11,"saveandexit"],[8,0],[7,[0,0]]],[[11,"sav_checked"],[8,1],[7,[0,-2]]]]
print("Original category 53 Save_run_progress dispatch and guards verified")
