#!/usr/bin/env python3
"""Compile the original create_map_location terrain event subtree to native C++.

No handwritten marker/range/edge catalog: every projected source paint, loop,
guard, variable assignment and terrain creation is compiled in event order.
Reject changed/unsupported expressions or conditions rather than dropping them.
The nonterrain sibling events remain the responsibility of the world interpreter.
"""
from pathlib import Path
from collections import Counter
import json
import re

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"data.deobfuscated.js"
OUTPUT=ROOT/"include/minidayz/generated/location_terrain_program.hpp"
GROUND={482,687,688,776,980,1138,1139}
TARGETS={1066,208,230}
CREATES=GROUND|{208,230}
VARS={"x","y","TilemapspotX","TilemapspotY"}
COMPARE=("==","!=","<","<=",">",">=")

def children(event):
    return [n for n in (event[7] if len(event)>7 else [])
            if isinstance(n,list) and len(n)>=7 and n[0]==0]

def walk(events):
    for e in events:
        if isinstance(e,list) and len(e)>=7 and e[0]==0:
            yield e
            yield from walk(children(e))

def paint(a):
    return a[0] in TARGETS and a[1] in (192,202)

def relevant(a):
    return (paint(a) or
            (a[:2]==[-1,94] and a[5][0][1] in CREATES) or
            (a[:2]==[-1,41] and a[5][0][1] in VARS) or
            (a[0]==208 and a[1] in (189,206)))

def contains(e,predicate):
    return any(predicate(a) for a in e[6]) or any(
        contains(n,predicate) for n in children(e))

def expr(e):
    op=e[0]
    if op==0:
        assert len(e)==2 and type(e[1]) in (int,float),e
        return str(e[1])+(".0" if type(e[1]) is int else "")
    if op==23:
        assert e[1] in VARS,e
        return "c."+e[1]
    if op==21:
        assert e[1:4]==[514,False,None] and e[4] in (0,1,2),e
        return {0:"c.marker",1:"c.row",2:"c.col"}[e[4]]
    if op in (4,5,6,7,12,13,14,15,16,17):
        symbol={4:"+",5:"-",6:"*",7:"/",12:"==",13:"!=",
                14:"<",15:"<=",16:">",17:">="}[op]
        return "("+expr(e[1])+symbol+expr(e[2])+")"
    if op==18:
        return "("+expr(e[1])+"?"+expr(e[2])+":"+expr(e[3])+")"
    if op==19:
        args=e[2] if len(e)>2 else []
        if e[1]==150:
            assert not args,e
            return "static_cast<double>(loopindex)"
        if e[1] in (80,204):
            assert len(args)==1,e
            return ("std::floor(" if e[1]==80 else "std::trunc(")+expr(args[0])+")"
        if e[1]==82:
            assert args and all(a[0]==0 for a in args),e
            return "c.choose({"+",".join(map(expr,args))+"})"
        if e[1]==95:
            assert len(args) in (1,2),e
            return "c.random("+",".join(map(expr,args))+")"
    raise ValueError(f"Unsupported terrain expression: {e}")

def value(p):
    assert p[0] in (0,7),p
    return expr(p[1])

def comparison(lhs,op,rhs):
    assert op[0]==8 and 0<=op[1]<len(COMPARE),op
    return "("+lhs+COMPARE[op[1]]+rhs+")"

def condition(c):
    obj,cmd=c[:2]
    p=c[9]
    if (obj,cmd)==(514,71):
        assert p[0][0]==10 and p[0][1] in (0,1,2),c
        return comparison({0:"c.marker",1:"c.row",2:"c.col"}[p[0][1]],p[1],value(p[2]))
    if (obj,cmd)==(-1,46):
        assert p[0][0]==11 and p[0][1] in ("CurrentLevel","perk_dao"),c
        return comparison({"CurrentLevel":"c.level","perk_dao":"c.perk"}[p[0][1]],p[1],value(p[2]))
    if (obj,cmd)==(1066,201):
        assert p==[[0,[21,514,False,None,6]]],c
        return "c.ground_object!=0"
    if (obj,cmd)==(518,203):
        return comparison("c.array_at("+value(p[0])+","+value(p[1])+")",p[2],value(p[3]))
    if obj in (1066,208) and cmd==205:
        return comparison("c.tile_at("+str(obj)+","+value(p[0])+","+value(p[1])+")",p[2],value(p[3]))
    raise ValueError(f"Unsupported terrain condition: {c}")

def origin(p,axis):
    e=p[1]
    assert e[:2]==[5,[20,514,axis,False,None]] and e[2][0]==0,p
    assert e[2][1] in (400,500),p
    return -e[2][1]

def main():
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
        SOURCE.read_text(encoding="utf-8-sig")))["project"]
    sheet=dict(project[6])["Game_events"]
    root=next(e for e in walk(sheet) if e[4]==5393337214081211)
    assert root[5][0][:2]==[514,92]
    # Confirm initial source tilemaps are plain ground or empty scatter.
    # The original first instance of each family member supplies default
    # artwork for newly created instances. Water explicitly clears itself.
    seen=set()
    for layout in project[5]:
        for layer in layout[6]:
            for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
                if not isinstance(inst,list) or len(inst)<6:continue
                obj=inst[1]
                if obj not in GROUND|{230} or obj in seen:continue
                seen.add(obj)
                cols,rows,rle=inst[0][13][:3]
                side=35 if obj==230 else 17
                assert cols>=side and rows>=side,(obj,cols,rows)
                cells=[]
                for part in rle.split(","):
                    q=part.split("x")
                    cells.extend([int(q[-1])]*(int(q[0]) if len(q)==2 else 1))
                expected=-1 if obj==230 else 6
                assert all(cells[y*cols+x]==expected
                           for y in range(side) for x in range(side)),obj
    assert seen==GROUND|{230},seen

    counts=Counter()
    lines=["#pragma once","#include <cmath>","#include <cstddef>",
           "namespace minidayz::world_source {",
           "// Generated from root original Game_events/create_map_location.",
           "template<class C> void execute_source_location_terrain(C& c) {"]

    def emit(line,depth):
        lines.append("    "*depth+line)

    def emit_action(a,depth):
        obj,cmd=a[:2]
        p=a[5]
        if paint(a):
            assert p[-1]==[3,0] and len(p)==(4 if cmd==192 else 6),a
            counts[obj,cmd]+=1
            emit("c.paint("+str(obj)+",{"+",".join(value(q) for q in p[:-1])+
                 "}); // source action "+str(a[3]),depth)
        elif (obj,cmd)==(-1,41):
            emit("c."+p[0][1]+"="+value(p[1])+";",depth)
        elif (obj,cmd)==(-1,94):
            assert p[0][0]==4 and p[1][0]==5 and p[1][1][0]==2,a
            target=p[0][1]
            assert p[1][1][1]==("bg" if target in GROUND else
                "Water" if target==208 else "ground_enviroment"),a
            emit(f"c.create({target},{origin(p[2],65)},{origin(p[3],66)});",depth)
        elif (obj,cmd)==(208,189):
            assert p==[[0,[0,1050]],[0,[0,1050]]],a
            # The native patch uses the exact 17x17 full-cell portion.
            emit("c.water_created=true;",depth)
        elif (obj,cmd)==(208,206):
            assert p==[[0,[0,0]],[0,[0,0]],[0,[0,17]],[0,[0,17]]],a
            emit("c.water.fill(-1);",depth)
        else:
            raise ValueError(f"Unimplemented projected action: {a}")

    def emit_siblings(nodes,depth):
        last=None
        for n in nodes:
            if not contains(n,relevant):
                last=None
                continue
            guards=n[5]
            is_else=any(c[:2]==[-1,70] for c in guards)
            repeat=[c for c in guards if c[:2]==[-1,123]]
            assert n[1] is None and n[3] is None,n[:5]
            if is_else:
                assert last=="if" and len(guards)==1,n[:6]
                emit("else {",depth)
                last="else"
            elif repeat:
                assert len(guards)==len(repeat)==1,n[:6]
                emit("for(int loopindex=0;loopindex<"+value(repeat[0][9][0])+
                     ";++loopindex) {",depth)
                last=None
            elif guards:
                joined=(" || " if n[2] else " && ").join(condition(c) for c in guards)
                emit("if("+joined+") {",depth)
                last="if"
            else:
                emit("{",depth)
                last=None
            for i,a in enumerate(n[6]):
                if not relevant(a):continue
                if a[:2]==[-1,94] and a[5][0][1] in GROUND:
                    target=a[5][0][1]
                    assert i+1<len(n[6]) and n[6][i+1][:2]==[514,88],a
                    assert n[6][i+1][5]==[[10,6],[7,[20,target,200,False,None]]],a
                emit_action(a,depth+1)
            emit_siblings(children(n),depth+1)
            emit("}",depth)

    # Restrict this native projection to original location terrain branches.
    # All other source actions remain in their respective runtime systems.
    branches=[n for n in children(root) if contains(n,paint)]
    emit_siblings(branches,1)
    assert counts==Counter({(1066,192):398,(1066,202):163,
                           (230,192):58,(208,192):12,(208,202):3}),counts
    lines+=["}", "inline constexpr std::size_t kSourceLocationTerrainPaintCount=634;"]
    # Compile the four remaining paint occurrences from their ORIGINAL
    # entrypoints. Bridge is a substate of Map, not a separate layout.
    # The tutorial and animated menu use their own authored Tilemap230.
    specs=[
        ("BridgeLoad","Game_events",895209268453600,333642970431083,250,
         [6835885414231505,678685540581074,482054460208135]),
        ("BridgeReload","Game_events",458228549101251,295534247678022,250,
         [6835885414231505,3280162310690592,793361912924261,441502873929221,606434895649703]),
        ("TutorialStart","Game_events",506106248958134,168499501893569,1250,
         [918656831349451,750535811603967]),
        ("MenuStart","Menu_Events",2084078328497493,7854813639071787,400,
         [6983171607608507,2477186353766075]),
    ]
    def chain_walk(events,parents=()):
        for e in events:
            if isinstance(e,list) and len(e)>=7 and e[0]==0:
                yield e,parents
                yield from chain_walk(children(e),parents+(e,))
    lines+=["enum class SourceSpecialTerrainEvent { BridgeLoad, BridgeReload, TutorialStart, MenuStart };",
            "inline constexpr std::size_t kSourceSpecialTerrainPaintCount=4;"]
    for name,sheet_name,uid,event_uid,repeats,ancestry in specs:
        matches=[(e,parents,a) for e,parents in chain_walk(dict(project[6])[sheet_name])
                 for a in e[6] if a[3]==uid and a[:2]==[230,192]]
        assert len(matches)==1,(uid,len(matches))
        e,parents,a=matches[0]
        assert e[4]==event_uid and [n[4] for n in parents]==ancestry
        assert len(e[5])==1 and e[5][0][:2]==[-1,123]
        assert e[5][0][9]==[[0,[0,repeats]]] and a[5][-1]==[3,0]
        if name=="BridgeReload":
            assert parents[-2][5][0][9]==[[11,"onBridge"],[8,0],[7,[0,1]]]
        if name.startswith("Bridge"):
            create=next(a for a in parents[-1][6] if a[:2]==[-1,94] and a[5][0]==[4,230])
            size=next(a for a in parents[-1][6] if a[:2]==[230,189])
            assert size[5]==[[0,[0,7838]],[0,[0,270]]]
            assert create[5][1:3]==[[5,[2,"ground_enviroment"]],[0,[23,"HotspotX"]]]
            assert create[5][3]==[0,[4 if name=="BridgeLoad" else 5,[23,"HotspotY"],[0,10 if name=="BridgeLoad" else 30]]]
        lines.append("template<class C> void execute_source_"+name+"(C& c) {")
        emit("for(int loopindex=0;loopindex<"+str(repeats)+";++loopindex) {",1)
        emit_action(a,2)
        emit("}",1)
        lines.append("}")
    assert sum(counts.values())==638,counts
    # Preserve the authored tilemap contents, not only the generated writes.
    # Source coordinates and frame/flip descriptors are in the root layout.
    for name,layout_name in (("TutorialStart","Tutorial"),("MenuStart","Menu")):
        layout=next(l for l in project[5] if l[0]==layout_name)
        matches=[(li,inst) for li,layer in enumerate(layout[6])
            for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list)
            if isinstance(inst,list) and len(inst)>=6 and inst[1]==230]
        assert len(matches)==1,(layout_name,len(matches))
        li,inst=matches[0]
        t=inst[0];cols,rows,rle=t[13][:3]
        assert inst[5][1:3]==[30,30]
        raw=[]
        for part in rle.split(","):
            q=part.split("x")
            raw.extend([int(q[-1])]*(int(q[0]) if len(q)==2 else 1))
        assert len(raw)==cols*rows
        lines.append(f"inline constexpr int k{name}Columns={cols}, k{name}Rows={rows};")
        lines.append(f"inline constexpr int k{name}Layer={li};")
        lines.append(f"inline constexpr float k{name}X={float(t[0])}f, k{name}Y={float(t[1])}f;")
        lines.append(f"inline constexpr float k{name}Width={float(t[3])}f, k{name}Height={float(t[4])}f;")
        lines.append(f"inline constexpr unsigned k{name}Authored[]={{")
        lines.extend("    "+",".join(str(v & 0xffffffff)+"u" for v in raw[i:i+24])+"," for i in range(0,len(raw),24))
        lines.append("};")
    # Root bridge base creation, not a guessed rectangle or the old
    # 318-only feature list.  First authored instance is Construct's
    # default instance for System.CreateObject.  Explicit source SetSize
    # and SetWidth replace those dimensions.
    bridge=next(e for e in walk(sheet) if e[4]==482054460208135)
    reload=next(e for e in walk(sheet) if e[4]==606434895649703)
    defaults={}
    for layout in project[5]:
        for layer in layout[6]:
            for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
                if isinstance(inst,list) and len(inst)>=6:
                    defaults.setdefault(inst[1],inst[0])
    def bridge_coord(p):
        e=p[1]
        if e[0]==23:
            return {"HotspotX":148,"HotspotY":18381}[e[1]]
        assert e[0] in (4,5) and e[1][0]==23 and e[2][0]==0,p
        return bridge_coord([0,e[1]])+(1 if e[0]==4 else -1)*e[2][1]
    pieces=[]
    current=None
    for a in bridge[6]:
        obj,cmd=a[:2]
        p=a[5] if len(a)>5 else []
        if (obj,cmd)==(-1,94):
            target=p[0][1]
            current=None
            if target not in (974,976,977,978,979,348):continue
            d=defaults[target]
            kind=project[3][target][1]
            assert kind in (14,18)
            png=Path(project[3][target][6][0]).name if kind==14 else ""
            current=dict(uid=a[3],object=target,kind=kind,frame=0,
                x=bridge_coord(p[2]),y=bridge_coord(p[3]),
                w=d[3],h=d[4],ox=d[8],oy=d[9],opacity=d[7],png=png,
                layer=p[1][1][1])
            pieces.append(current)
        elif current is not None and obj==current["object"]:
            if cmd in (103,190):
                current["w"],current["h"]=[q[1][1] for q in p]
            elif cmd==156:
                current["w"]=p[0][1][1]
            elif cmd==72:
                current["frame"]=p[0][1][1]
    assert len(pieces)==10 and sum(p["object"]==348 for p in pieces)==4,pieces
    assert all(p["frame"]==26 for p in pieces if p["object"]==348)
    # The original save restores bridge art, then the reload branch
    # recreates the four Solid sprites with fresh source action IDs.
    reload_solids=[]
    for i,a in enumerate(reload[6]):
        if a[:2]==[-1,94] and a[5][0]==[4,348]:
            assert reload[6][i+1][:2]==[348,72] and reload[6][i+1][5]==[[0,[0,26]]]
            assert reload[6][i+2][:2]==[348,103]
            reload_solids.append((a[3],bridge_coord(a[5][2]),bridge_coord(a[5][3]),
                                  *[q[1][1] for q in reload[6][i+2][5]]))
    solids=[p for p in pieces if p["object"]==348]
    assert len(reload_solids)==4
    for p,q in zip(solids,reload_solids):
        assert (p["x"],p["y"],p["w"],p["h"])==q[1:]
        p["reload_uid"]=q[0]
    lines+=["struct SourceBridgePiece { unsigned long long uid,reload_uid; unsigned short object,frame; unsigned char kind; float x,y,w,h,ox,oy,opacity; const char* layer; const char* png; };",
            "inline constexpr SourceBridgePiece kSourceBridgeBasePieces[]={"]
    for p in pieces:
        vals=[str(p["uid"])+"ULL",str(p.get("reload_uid",p["uid"]))+"ULL",
              str(p["object"]),str(p["frame"]),str(p["kind"])]
        vals += [str(float(p[k]))+"f" for k in ("x","y","w","h","ox","oy","opacity")]
        vals += [json.dumps(p["layer"]),json.dumps(p["png"])]
        lines.append("    {"+",".join(vals)+"},")
    lines+=["};","inline constexpr std::size_t kSourceBridgeBasePieceCount=10;",
            "inline constexpr std::size_t kSourceTotalTerrainPaintCount=638;","}"]
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text("\n".join(lines)+"\n",encoding="utf-8")
    print("Compiled original location and special-context terrain:",dict(counts),
          "total",sum(counts.values()),"source actions")

if __name__=="__main__":
    main()
