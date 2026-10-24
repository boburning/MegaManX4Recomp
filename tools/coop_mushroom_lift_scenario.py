"""Bounded native high-jump camera check before the authentic ceiling crush.

Checkpoint, acquired-ability and fatal-damage fixtures prepare the riders.
Jump/attack input, lift movement, camera controls and collision remain native.
This checks the reported drift; it does not claim ceiling-panel clearance.
"""
import json
import struct
import time
from coop_debug import capture_pair


def mushroom_lift_crush(s):
    """Trace the reported idle position through real lift/ceiling collision.

    An observation probe: no panels, collision data or death outcomes are set.
    Damage protection excludes enemy hits but deliberately preserves crushes.
    """
    r=s.r
    s.load(3,1,2);r.fixture(5,3)
    initial=s.wait('lift-crush-riders',lambda v:all(p['grounded'] for p in v['players']),15)
    for seat,x in ((0,6030),(1,6052)):
        r.teleport(seat,x,round(initial['players'][seat]['y']))
    r.request('wtrace_range',lo='0x80141924',hi='0x80141925')
    r.request('wtrace_clear')
    rows=[];started=time.monotonic()
    try:
        while time.monotonic()-started<120:
            row=s.sample('lift-crush-%d'%len(rows))
            row['contacts']=[list(b[0x70:0x7a]) for b in r.players()]
            lift=r.read(0x80165a30,32*0x8c)
            row['lifts']=[dict(slot=n,phase=list(b[4:8]),
                x=struct.unpack_from('<i',b,8)[0]/65536,
                y=struct.unpack_from('<i',b,12)[0]/65536)
                for n in range(32) if (b:=lift[n*0x8c:(n+1)*0x8c])[0] and b[1]==14]
            rows.append(row)
            if len(rows)%60==1 and getattr(s,'capture_composed',True):
                capture_pair(r,s.output/('lift-creep-%03d.png'%len(rows)))
            if any(p['hp']==0 for p in row['players']):
                (s.output/'lift-terminal-writes.json').write_text(json.dumps(
                    r.request('wtrace_dump',count=4096,newest=1),indent=2))
                if getattr(s,'capture_composed',True):capture_pair(r,s.output/'lift-creep-terminal.png')
                break
            time.sleep(.06)
    finally:
        r.release()
        (s.output/'lift-creep-observations.json').write_text(json.dumps(rows,indent=2)+'\n')
    return dict(observation_only=True,terminal=rows[-1],panels_cleared=False)


def mushroom_lift_high_jump(s):
    r = s.r
    state = next(c['count'] for c in r.request('mod_counters')['counters']
                 if c['name'] == 'mmx4.coop.split-state')
    assert state, 'This regression requires Split cameras'
    results = []
    for survivor in (False, True):
        s.load(3, 1, 2)
        r.fixture(5, 3)
        zero = s.campaign ^ 1
        r.fixture(7, zero, 0, 1)
        initial = s.wait('lift-native-riders', lambda v: not v['blocked'] and all(
            p['hp'] == 32 and p['grounded'] and p['action'] == 2
            for p in v['players']), 15)
        if survivor:
            r.fixture(2, 0, 0x80)
            s.wait('lift-p1-dead', lambda v: v['players'][0]['state'] == 3, 15)
        jumper = 1 if survivor else zero
        rows = []
        start = time.monotonic()
        last_input = None
        captured = False
        try:
            while time.monotonic() - start < 20:
                age = time.monotonic() - start
                phase = age % 1.5
                jump = phase < .3 or .4 < phase < .8
                fire = int(age * 8) % 2 == 0
                mask = 0xffff & (~0x4000 if jump else 0xffff) & (~0x8000 if fire else 0xffff)
                if mask != last_input:
                    r.input(mask, jumper)
                    last_input = mask
                data = r.read(r.diagnostic(), 0x6d0)
                cameras = r.read(state + 16, 2*0xfc)
                players = [dict(hp=data[a+0x5c] & 127, state=data[a+4], action=data[a+5],
                    y=struct.unpack_from('<i', data, a+12)[0]/65536)
                    for a in (0x300, 0x100)]
                row = dict(age=age, frame=struct.unpack_from('<I', data, 4)[0], players=players,
                    cameras=[struct.unpack_from('<i', cameras, a+12)[0]/65536 for a in (0, 0xfc)])
                rows.append(row)
                assert players[1]['hp'] == 32, row
                assert players[0]['hp'] == (0 if survivor else 32), row
                assert abs(row['cameras'][0] - row['cameras'][1]) <= 2, ('Lift camera drift', row)
                if not survivor and players[jumper ^ 1]['y'] - players[jumper]['y'] >= 100 and not captured:
                    capture_pair(r, s.output/('lift-high-jump-survivor%d.png' % survivor))
                    captured = True
                time.sleep(.025)
        finally:
            r.release()
            (s.output/('lift-high-jump-survivor%d.json' % survivor)).write_text(json.dumps(rows, indent=2)+'\n')
        if not survivor:
            assert captured, 'Native controls did not reproduce a >=100px high jump'
        after = s.wait('lift-rider-controls-restored', lambda v: not v['blocked'] and
            v['players'][1]['hp'] == 32 and v['players'][1]['grounded'] and
            v['players'][1]['action'] == 2, 8)
        assert after['lives'] == initial['lives']
        capture_pair(r, s.output/('lift-end-survivor%d.png' % survivor))
        results.append(dict(surviving_p2_only=survivor, jumper=jumper, after=after,
            samples=len(rows), maximum_camera_separation=max(abs(v['cameras'][0]-v['cameras'][1]) for v in rows)))
    return dict(cases=results, ceiling_panel_clearance=False)
