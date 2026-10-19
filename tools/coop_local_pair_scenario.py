"""Bounded native local side-by-side checks; capture the composed drawable."""
import time
import json
import struct


def local_pair(self):
    r=self.r
    r.request('window_size',w=1280,h=480)
    def capture(label):
        path=self.output/(label+'.png')
        before=r.request('present_shot_seq')['seq']
        r.request('present_shot',path=str(path))
        deadline=time.monotonic()+10
        while time.monotonic()<deadline:
            result=r.request('present_shot_seq')
            if result['seq']!=before:
                assert result['wrote'] and path.is_file(),result
                return str(path)
            time.sleep(.03)
        raise TimeoutError('No composed presentation: '+label)
    def counters():
        return {p['name']:p['count'] for p in r.request('mod_counters')['counters']}
    def cameras_settled(label):
        address=counters()['mmx4.coop.split-state']
        def ready(row):
            cameras=r.read(address+16,2*0xfc)
            result=[]
            for seat,body in enumerate(row['players']):
                x,y=struct.unpack_from('<h',cameras,seat*0xfc+10)[0],struct.unpack_from('<h',cameras,seat*0xfc+14)[0]
                result.append(48<=body['x']-x<=272 and 16<=body['y']-y<=224)
            return all(result)
        return self.wait(label,ready,20)
    self.load(1,0)
    r.teleport_both(500,427)
    r.teleport(1,556,427)
    close=cameras_settled('local-close-settled')
    pictures=[capture('local-close')]
    # Controller-driven separation exercises native ring/placement admission;
    # jumping 700px with a fixture can leave the retail ring partly stale too.
    deadline=time.monotonic()+24
    try:
        while True:
            row=self.sample('local-separation-input')
            assert all(p['hp'] for p in row['players']),row
            if row['players'][1]['x']-row['players'][0]['x']>660:break
            if time.monotonic()>deadline:raise TimeoutError('P2 did not move far enough')
            r.input(0x9fdf,1);time.sleep(.4)
            r.input(0xffdf,1);time.sleep(.25)
    finally:r.release()
    distant=cameras_settled('local-distant-settled')
    time.sleep(.3)
    pictures.append(capture('local-distant'))
    assert all(p['hp'] for p in distant['players']),distant
    assert abs(distant['players'][0]['x']-distant['players'][1]['x'])>600,distant
    before=counters().get('mmx4.coop.local-pair-committed',0)
    r.input(0xffdf,1);time.sleep(.35);r.release()
    time.sleep(.2)
    (self.output/'pair-diagnostic.json').write_text(json.dumps(dict(render=r.request('render_pass_stats'),stereo=r.request('stereo_stats'),gpu=r.request('gpu_state')),indent=2))
    assert counters().get('mmx4.coop.local-pair-committed',0)>before,counters()
    pictures.append(capture('local-moving'))
    r.input(0xfff7,0);time.sleep(.15);r.release()
    self.wait('local-paused',lambda v:v['mode'][:2]==[6,2],5)
    time.sleep(1)
    pictures.append(capture('local-pause'))
    deadline=time.monotonic()+8
    while r.read(0x801721c0,2)!=bytes([6,0]):
        if time.monotonic()>deadline:raise TimeoutError('Native pause did not close')
        r.input(0xfff7,0);time.sleep(.2);r.release();time.sleep(.5)
    self.wait('local-resumed',lambda v:v['mode'][:2]==[6,0],5)
    r.request('window_size',w=960,h=720)
    time.sleep(.3)
    pictures.append(capture('local-resized'))
    stats=r.request('render_pass_stats')
    counts=counters()
    assert counts.get('mmx4.coop.local-pair-committed',0)>0,counts
    assert not counts.get('mmx4.coop.local-view-state-leak',0),counts
    assert stats['verify_checks']>0 and stats['verify_mismatch']==0,stats
    assert stats['aborted']==stats['watchdog']==stats['vram_leaks']==0,stats
    return dict(close=close,distant=distant,pictures=pictures,render_pass=stats,counters=counts)
