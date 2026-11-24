#!/usr/bin/env python3
"""Compile deterministic root create_map_element category 4/6 event trees.

All source branch predicates and action parameters are interpreted in order.
Unsupported predicates/actions abort extraction, rather than dropping scenery.
Output contains source frame geometry, transforms, UID links and loot flags.
No RNG or invented scenery is used by these two original categories.
"""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]

def walk(events):
    for e in events:
        if isinstance(e,list) and len(e)>=7 and e[0]==0:
            yield e
            yield from walk(e[7] if len(e)>7 else [])

def compile_source(p):
    objects=p[3]
    events={e[4]:e for e in walk(dict(p[6])["Game_events"])}
    groups={4:events[1506402191387381],6:events[4174243855126541]}
    visited=set()
    def frame_data(obj,frame):
        return objects[obj][7][0][7][frame]
    def run(cat,var,sub,state):
        rows=[]
        values={1:cat,2:var,3:state,6:sub}
        def current(obj):
            return next(i for i in range(len(rows)-1,-1,-1) if rows[i]['object']==obj)
        def point(i,name):
            if i<0:return 0.,0.
            row=rows[i];f=frame_data(row['object'],row['frame']);pts=f[9]
            pt=None
            if isinstance(name,int) and 1<=name<=len(pts):pt=pts[name-1]
            elif isinstance(name,str):pt=next((v for v in pts if v[0].lower()==name.lower()),None)
            if pt is None:return row['x'],row['y']
            return row['x']+(pt[1]-f[7])*row['w']*row['sx'],row['y']+(pt[2]-f[8])*row['h']
        def expr(e):
            op=e[0]
            if op in (0,2):return e[1]
            if op==4:return expr(e[1])+expr(e[2])
            if op==5:return expr(e[1])-expr(e[2])
            if op==7:return expr(e[1])/expr(e[2])
            if op==21 and e[1]==508:return values[e[4]]
            if op==20:
                obj,code=e[1:3];i=current(obj);row=rows[i]
                if code==151:return ('uid',i)
                if code in (59,60):return point(i,expr(e[5][0]))[code-59]
                if code==61:return row['w']*row['sx']
                if code==66:return row['x']
            raise AssertionError(('unsupported expression',e))
        def execute(event):
            for c in event[5]:
                assert not any(c[i] for i in (4,5,6)),c
                a=c[9]
                if c[:2]==[508,71] and a[1]==[8,0]:
                    if a[0][1]==6:assert 0<=expr(a[2][1])<=6,c
                    if a[0][1]==3:assert expr(a[2][1]) in (0,1),c
                    if values.get(a[0][1])!=expr(a[2][1]):return
                elif c[1]==64 and a[0]==[8,0]:
                    if rows[current(c[0])]['frame']!=expr(a[1][1]):return
                else:raise AssertionError(('unsupported condition',c))
            visited.add(event[4])
            for a in event[6]:
                obj,op=a[:2];args=a[5]
                if op==73:
                    assert args[0][0]==4 and args[1][0]==5 and args[2][0]==7,a
                    child=args[0][1];parent=-1 if obj==508 else current(obj)
                    x,y=point(parent,expr(args[2][1]));f=frame_data(child,0)
                    rows.append(dict(event=event[4],action=a[3],object=child,
                        layer=expr(args[1][1]),parent=parent,frame=0,x=x,y=y,
                        w=float(f[4]),h=float(f[5]),ox=f[7],oy=f[8],sx=1.,
                        v0=0,v1=0,loot=False))
                elif obj==508 and op==88:
                    # Map element var0/5 stores loaded flag/primary UID.
                    # Preserve source state changes when evaluating children.
                    slot=args[0][1];assert slot in (0,3,5),a
                    if slot==3:values[slot]=expr(args[1][1])
                elif obj==189 and op==69:
                    assert expr(args[0][1])=='Trigger_spawn' and expr(args[1][1][1])==('uid',current(218)),a
                    rows[current(218)]['loot']=True
                else:
                    row=rows[current(obj)]
                    if op==72:
                        if args[0][1][0]==21:
                            assert len(objects[obj][7][0][7])<=8,a
                        frame=max(0,min(len(objects[obj][7][0][7])-1,int(expr(args[0][1]))))
                        old=frame_data(obj,row['frame']);new=frame_data(obj,frame)
                        row.update(frame=frame,w=row['w']*new[4]/old[4],
                                   h=row['h']*new[5]/old[5],ox=new[7],oy=new[8])
                    elif op==88:
                        slot=args[0][1];assert slot in (0,1),a
                        row['v'+str(slot)]=expr(args[1][1])
                    elif op==161:
                        slot=args[0][1];assert slot in (0,1) and args[1][1] in (0,1),a
                        row['v'+str(slot)]=args[1][1]
                    elif op==159:
                        assert args[0][1] in (0,1),a
                        row['sx']=-1. if args[0][1]==0 else 1.
                    elif op==103:
                        w,h=expr(args[0][1]),expr(args[1][1])
                        row.update(w=abs(w),h=h,sx=-1. if w<0 else 1.)
                    elif op==169:row['h']=expr(args[0][1])
                    elif op==55:row['x']=expr(args[0][1])
                    else:raise AssertionError(('unsupported action',a))
            for child in event[7] if len(event)>7 else []:
                if isinstance(child,list) and len(child)>=7 and child[0]==0:execute(child)
        execute(groups[cat])
        for row in rows:
            assert row['w']>0 and row['h']>0,row
        return rows
    # 7 represents all larger subtypes: literal subtype guards stop at 6,
    # and dynamic frame assignment clamps at or below frame 7.
    # state class 2 represents values neither exactly 0 nor exactly 1.
    rows=[];cases=[];unique={}
    for cat,last in ((4,48),(6,17)):
        for v in range(1,last+1):
            for sub in range(8):
                for state in range(3):
                    sequence=run(cat,v,sub,state)
                    key=repr(sequence)
                    if key not in unique:
                        unique[key]=(len(rows),len(sequence));rows.extend(sequence)
                    first,count=unique[key]
                    cases.append((cat,v,sub,state,first,count))
    required={e[4] for g in groups.values() for e in walk([g]) if e[6]}
    assert required<=visited,('unreached source action branches',required-visited)
    return rows,cases,len(required)

def output(p,path):
    rows,cases,events=compile_source(p)
    def fl(x):
        s=format(float(x),'.9g')
        return s+('f' if '.' in s or 'e' in s else '.0f')
    def var(x):
        if isinstance(x,tuple):return str(-x[1]-1)
        assert isinstance(x,int) and x>=0,x
        return str(x)
    lines=['#pragma once','#include <cstdint>','#include <cstddef>',
      'namespace minidayz::world_source {',
      '// Generated directly from root category4/6 source event actions.',
      '// Negative var0/1 encode -(local selected source UID index+1).',
      'struct Category46Placement {std::uint64_t event_uid,action_uid;std::uint16_t object;const char* layer;std::uint16_t frame;int parent;float x,y,w,h,ox,oy,sx;int var0,var1;bool loot;};',
      'struct Category46Case {std::uint8_t category,variant,subtype,state;std::uint16_t first,count;};',
      'inline constexpr Category46Placement kCategory46Placements[]={']
    for r in rows:
        lines.append('{'+','.join([str(r['event'])+'ULL',str(r['action'])+'ULL',str(r['object']),json.dumps(r['layer']),str(r['frame']),str(r['parent']),*(fl(r[k]) for k in ('x','y','w','h','ox','oy','sx')),var(r['v0']),var(r['v1']),'true' if r['loot'] else 'false'])+'},')
    lines+=['};','inline constexpr Category46Case kCategory46Cases[]={']
    lines+=['{'+','.join(map(str,c))+'},' for c in cases]
    lines+=['};','} // namespace minidayz::world_source','']
    path.parent.mkdir(parents=True,exist_ok=True);path.write_text('\n'.join(lines),encoding='utf-8')
    print('PASS original category4/6',events,'action-bearing source events;',len(rows),'unique placement rows;',len(cases),'input cases')

if __name__=='__main__':
    source=(ROOT/'data.deobfuscated.js').read_text(encoding='utf-8-sig')
    p=json.loads(re.sub(r'(?<![A-Za-z0-9_.])-\s+(?=\d)','-',source))['project']
    output(p,ROOT/'include/minidayz/generated/category46_source.hpp')
