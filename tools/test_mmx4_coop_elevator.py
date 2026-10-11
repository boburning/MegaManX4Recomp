"""Qualify native Mushroom lift arrivals and retries in a private debug copy."""
import argparse,json,struct,time
from pathlib import Path
from coop_debug import Runtime

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port',type=int,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--campaign',type=int,choices=(0,1),default=0)
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    r=Runtime(args.port);events=[]
    def observe():
        d=r.read(r.diagnostic(),0x464);play=r.read(0x801721c0,0x64)
        camera=r.read(0x801419b0,0x54)
        return dict(frame=struct.unpack_from('<I',d,4)[0],enrolled=d[8],failed=d[12],
            mode=play[0],phase=play[1],stage=play[12],section=play[13],checkpoint=play[0x1d],
            lives=play[0x44],gate=play[0x1c],camera_y=struct.unpack_from('<h',camera,14)[0],
            camera_maximum_y=struct.unpack_from('<h',camera,0x28)[0],
            players=[dict(active=b[0],char=b[2],hp=b[0x5c]&127,state=b[4],action=b[5],
                          x=struct.unpack_from('<i',b,8)[0]/65536,y=struct.unpack_from('<i',b,12)[0]/65536)
                     for b in (d[0x300:0x3e4],d[0x100:0x1e4])])
    def wait(label,predicate,timeout=60,healthy=False):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            s=observe()
            assert not s['failed'],s
            if healthy:assert all(b['hp']==32 for b in s['players']),s
            if predicate(s):
                s['label']=label;events.append(s)
                (args.output/'report.json').write_text(json.dumps(events,indent=2))
                r.screenshot(args.output/(label+'.png'));print(json.dumps(s),flush=True)
                return s
            time.sleep(.06)
        raise TimeoutError((label,s))
    def live(s):
        return s['mode']==6 and not s['phase'] and not s['gate'] and s['enrolled'] and \
            all(b['active'] and b['hp']==32 and b['state']==1 and b['action']==2 for b in s['players'])
    try:
        r.release();r.fixture(5,3);r.fixture(1,3,1,args.campaign)
        r.wait_playable(60,advance_dialogue=True)
        r.fixture(3,2)
        start=wait('lift-arrival',live)
        r.fixture(5,3)
        assert [b['char'] for b in start['players']]==[args.campaign,args.campaign^1]
        ascent=wait('lift-destination-before-camera',lambda s:live(s) and s['camera_maximum_y']<1000 and s['camera_y']>1800,healthy=True)
        assert ascent['lives']==start['lives']
        # Retry from the same low lift spawn, not from its completed top.
        r.fixture(2,0,0x80);r.fixture(2,1,0x80)
        retry=wait('native-lift-retry',lambda s:live(s) and s['lives']==start['lives']-1)
        r.fixture(5,3)
        wait('retry-destination-before-camera',lambda s:live(s) and s['camera_maximum_y']<1000 and s['camera_y']>1800,healthy=True)
        # The original lift requires players to destroy overhead panels.
        # Owner reviewed QA34 and confirmed that idling into them should crush
        # the riders. Qualify the scroll regression before those obstacles.
        rising=wait('retry-continued-ascent',lambda s:live(s) and s['camera_y']<1750 and
                    all(b['y']<1950 for b in s['players']),timeout=30,healthy=True)
        assert rising['lives']==retry['lives']
        assert rising['camera_maximum_y']==720
        assert abs(rising['players'][0]['x']-rising['players'][1]['x'])<=24
        print('PASS: both riders survive destination-limit change and continued ascent; team retry spends one life',flush=True)
    finally:r.release()

if __name__=='__main__':main()
