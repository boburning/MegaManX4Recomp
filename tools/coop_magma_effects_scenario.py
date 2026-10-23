"""Observe native Volcano fireball controllers with a distant living partner.

Positions and damage protection only prepare the scene. All emitter state,
projectiles, terrain hits and shake commands are produced by original code.
"""
import json
import struct
import time
from coop_debug import capture_pair


def magma_idle_resolution(s):
    """Isolate host pixel cost at neutral part2 start; no simulation changes."""
    r=s.r;s.load(4,1);r.fixture(5,3)
    original=r.request('video_info')
    profiles=[]
    try:
        for scale in ((6,3,6) if original['dynamic'] else (None,)):
            if scale:r.request('dynres_force',scale=scale)
            time.sleep(3)
            samples=[]
            for i in range(15):
                data=r.read(r.diagnostic(),16)
                samples.append(dict(t=time.monotonic(),frame=struct.unpack_from('<I',data,4)[0]))
                time.sleep(1)
            profiles.append(dict(requested_scale=scale,video=r.request('video_info'),
                samples=samples,perf=r.request('frame_perf'),phase=r.request('phase_profile',window=10)))
    finally:
        if original['dynamic']:r.request('dynres_force',scale=0)
        (s.output/'magma-idle-resolution.json').write_text(json.dumps(
            dict(original=original,profiles=profiles),indent=2)+'\n')
    return dict(observation_only=True,scales=[p['video']['effective_scale'] for p in profiles])


def magma_effects(s,field=False):
    r=s.r
    state=next(c['count'] for c in r.request('mod_counters')['counters']
               if c['name']=='mmx4.coop.split-state')
    results=[]
    cases=[(part,leader) for part in ((0,1) if field else (0,)) for leader in (1,0)]
    for part,leader in cases:
        s.load(4,part)
        r.fixture(5,3)
        folder=s.output/('magma-part%d-leader-%d'%(part+1,leader));folder.mkdir()
        rows=[]
        if part==1:
            # Neutral original arrival, before any hero can trigger an emitter.
            # Frame rate/phase attribution is bounded to this idle control.
            samples=[]
            for i in range(15):
                start=time.monotonic();d=r.read(r.diagnostic(),16)
                samples.append(dict(t=start,frame=struct.unpack_from('<I',d,4)[0]))
                time.sleep(1)
            profile=dict(samples=samples,phase=r.request('phase_profile',window=10),gpu=r.request('gpu_state'))
            try:profile['frame_perf']=r.request('frame_perf')
            except RuntimeError as e:profile['frame_perf_error']=str(e)
            (folder/'idle-profile.json').write_text(json.dumps(profile,indent=2)+'\n')
        # These are observation probes; unsafe setup positions are reported
        # explicitly and cannot count as a gameplay failure or a pass.
        probes=(((600,300),(2112,300),(3400,700)) if part==0 else
                ((480,400),(2050,300))) if field else ((1600,300),(2400,500),(3400,700))
        for x,y in probes:
            r.teleport(leader,x,y)
            time.sleep(3)
            for i in range(75):
                row=s.sample('magma-%d-%d-%d'%(leader,x,i))
                for name,base,count,stride in (('controllers',0x80142f98,32,0x30),
                                               ('fireballs',0x8013f328,32,0x9c)):
                    data=r.read(base,count*stride)
                    row[name]=[dict(address=hex(base+n*stride),type=b[1],subtype=b[2],
                         phase=list(b[4:8]),x=struct.unpack_from('<i',b,8)[0]/65536,
                         y=struct.unpack_from('<i',b,12)[0]/65536,raw=b.hex())
                         for n in range(count) if (b:=data[n*stride:(n+1)*stride])[0]
                         and (name=='controllers' or b[1]==33)]
                cameras=r.read(state+16,504)
                row['shake']=[list(cameras[n*252+0x34:n*252+0x48]) for n in (0,1)]
                rows.append(row)
                if i%25==0:(folder/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
                if row['players'][leader]['hp']==0:
                    row['excluded']='Unsafe setup position; no gameplay inference'
                    break
                if i in (0,25,74):capture_pair(r,folder/('%d-%02d.png'%(x,i)))
                time.sleep(.1)
            if rows[-1]['players'][leader]['hp']==0:break
        (folder/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
        results.append(dict(part=part+1,leader=leader,observations=len(rows),observation_only=True))
    return dict(native_probes=results)
