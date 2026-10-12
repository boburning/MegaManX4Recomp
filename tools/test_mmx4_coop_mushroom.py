"""Native Mushroom entrance-platform and P2-led room transition regressions.

Connect only to an owned hidden development instance with private cards.
Fixtures load original stage/checkpoint assets and native fatal deaths;
controller input crosses the actual exit trigger. Requires the original disc.
"""
import argparse,json,struct,time
from pathlib import Path
from coop_debug import Runtime

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port',type=int,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--campaign',type=int,choices=(0,1),default=0)
    p.add_argument('--portal-return',action='store_true',help='Also qualify the separately tracked initial green-portal return')
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    r=Runtime(args.port);events=[]
    def observe(label):
        d=r.read(r.diagnostic(),0x464);play=r.read(0x801721c0,0x64)
        return dict(label=label,frame=struct.unpack_from('<I',d,4)[0],mode=play[0],phase=play[1],
            stage=play[12],section=play[13],checkpoint=play[0x1d],lives=play[0x44],gate=play[0x1c],
            players=[dict(active=b[0],state=b[4],action=b[5],hp=b[0x5c]&127,ground=b[0x89],
                x=struct.unpack_from('<i',b,8)[0]/65536,y=struct.unpack_from('<i',b,12)[0]/65536,
                box=struct.unpack_from('<I',b,0x68)[0]) for b in (d[0x300:0x3e4],d[0x100:0x1e4])])
    def record(label):
        s=observe(label);events.append(s)
        (args.output/'report.json').write_text(json.dumps(events,indent=2)+'\n')
        r.screenshot(args.output/(label+'.png'));print(json.dumps(s),flush=True);return s
    def platform(label):
        s=record(label);actors=r.read(0x80165a30,32*0x8c)
        portal=next(b for i in range(32) if (b:=actors[i*0x8c:(i+1)*0x8c])[0] and b[1]==11 and b[2]&128)
        box=r.read(struct.unpack_from('<I',portal,0x68)[0],4)
        top=struct.unpack_from('<i',portal,12)[0]/65536+struct.unpack_from('<b',box,1)[0]-box[3]
        for b in s['players']:
            assert b['active'] and b['state']==1 and b['hp']==32 and b['ground']&8,s
            box=r.read(b['box'],4);bottom=b['y']+struct.unpack_from('<b',box,1)[0]+box[3]
            assert abs(bottom-top)<=1,(label,'player fell through entrance platform',bottom,top,s)
        return s
    try:
        r.release();r.fixture(5,3);r.fixture(1,3,0,args.campaign);r.wait_playable(45,True)
        if args.portal_return:
            r.teleport_both(1920,234);r.teleport(1,2016,140)
            deadline=time.monotonic()+20
            while r.read(0x801721dd,1)[0]!=1:
                assert time.monotonic()<deadline,'P2 did not commit green portal'
                time.sleep(.05)
            r.wait_playable(45,False);r.fixture(5,3);r.fixture(2,0,32);r.fixture(2,1,32)
            platform('p2-portal-return')
        r.fixture(3,1);r.wait_playable(45,True)
        r.fixture(5,3);r.fixture(2,0,32);r.fixture(2,1,32);time.sleep(.3)
        before=platform('checkpoint-platform')
        r.input(0xbfff,0);time.sleep(.2);r.release();time.sleep(1)
        platform('jump-land-platform')
        r.fixture(2,0,0x80);r.fixture(2,1,0x80);r.wait_playable(45,True)
        r.fixture(5,3)
        retry=platform('team-retry-platform');assert retry['lives']==before['lives']-1,retry
        r.teleport_both(3380,203);r.teleport(1,3480,110);time.sleep(.7)
        record('before-p2-exit');r.input(0xffdf,1)
        deadline=time.monotonic()+15
        while True:
            s=observe('p2-exit')
            assert s['mode']!=6 or s['phase'] or all(b['hp'] for b in s['players']),s
            if s['checkpoint']==2:break
            assert time.monotonic()<deadline,('P2 did not advance native checkpoint',s)
            time.sleep(.04)
        r.release();r.wait_playable(45,True)
        after=record('p2-exit-arrival')
        assert after['checkpoint']==2 and after['lives']==retry['lives'],after
        assert all(b['hp']==32 for b in after['players']),after
        print('PASS: both characters land on the native portal platform, team retry spends one life, and P2 advances the exit safely',flush=True)
    finally:r.release()

if __name__=='__main__':main()
