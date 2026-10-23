"""Observe both leaders' actual first Eregion flyby in independent views.

Only initial stage/position/invulnerability fixtures are used. Controller input
crosses the encounter; no dragon, camera, phase or completion field is written.
Composed images and native actor/camera snapshots require visual assessment.
"""
import json
import struct
import time
from coop_debug import capture_pair


def intro_dragon_views(s,restore=False):
    r = s.r
    state = next(c['count'] for c in r.request('mod_counters')['counters']
                 if c['name'] == 'mmx4.coop.split-state')
    assert state, 'Independent cameras required'
    results = []
    for leader in ((1,) if restore else (1,0)):
        s.load(0, 0)
        r.fixture(5, 3)
        r.teleport(leader, 1200, 427)
        time.sleep(3)
        folder = s.output / ('dragon-leader-%d' % leader)
        folder.mkdir()
        rows, captures, phases = [], 0, set()
        restored=False
        started = time.monotonic()
        r.input(0xffdf, leader)
        try:
            while time.monotonic() - started < (110 if restore else 75):
                row = s.sample('dragon-%d-%d' % (leader, len(rows)))
                cameras = r.read(state + 16, 504)
                row['views'] = [dict(x=struct.unpack_from('<h', cameras, n*252+10)[0],
                                     y=struct.unpack_from('<h', cameras, n*252+14)[0]) for n in (0, 1)]
                row['canonical'] = r.read(0x801419b0, 252).hex()
                shots=r.read(0x8013f328,32*0x9c)
                row['shots']=[dict(slot=n,subtype=b[2],phase=list(b[4:8]),layer=b[0x14],
                    x=struct.unpack_from('<i',b,8)[0]/65536,y=struct.unpack_from('<i',b,12)[0]/65536)
                    for n in range(32) if (b:=shots[n*0x9c:(n+1)*0x9c])[0] and b[1]==0]
                row['actors'] = []
                pool = r.read(0x8013bed0, 48*0x9c)
                for n in range(48):
                    body = pool[n*0x9c:(n+1)*0x9c]
                    if body[0] and body[1] == 0:
                        row['actors'].append(dict(address=hex(0x8013bed0+n*0x9c),
                            state=list(body[4:8]),layer=body[0x14],visible=body[3],
                            x=struct.unpack_from('<i',body,8)[0]/65536,
                            y=struct.unpack_from('<i',body,12)[0]/65536,body=body.hex()))
                row['seconds'] = time.monotonic()-started
                rows.append(row)
                dragon = next((a for a in row['actors'] if a['visible']), None)
                if dragon:
                    phase = tuple(dragon['state'])+(dragon['layer'],)
                    if phase not in phases or (dragon['layer']==255 and len(rows)%12 == 0):
                        if captures < 48 and getattr(s,'capture_composed',True):
                            capture_pair(r,folder/('flyby-%03d.png' % len(rows)))
                            captures += 1
                    phases.add(phase)
                    if restore and not restored and dragon['state'][1]==4 and dragon['layer']==255:
                        r.release()
                        r.request('savestate',op='save',slot=11)
                        deadline=time.monotonic()+10
                        while not list(s.output.glob('saves/*/state_*_slot11.pst')):
                            assert time.monotonic()<deadline,'Snapshot was not written'
                            time.sleep(.1)
                        # Reset the actor and owner records through an actual
                        # new stage, then restore the same build's corner phase.
                        s.load(0,0)
                        r.request('savestate',op='load',slot=11)
                        time.sleep(1)
                        if getattr(s,'capture_composed',True):capture_pair(r,folder/'restored-corner.png')
                        restored=True
                        r.input(0xffdf,leader)
                # Stop before advancing to unrelated stage controllers.
                if any(p[1]==4 and p[-1]==255 for p in phases) and dragon and \
                        dragon['layer']==0 and len(phases) >= 8:
                    break
                time.sleep(.12)
        finally:
            try:r.release()
            except OSError:pass
            (folder/'observations.json').write_text(json.dumps(rows,indent=2)+'\n')
        assert phases, 'No native dragon was observed'
        assert all(v['players'][leader^1]['x'] < 300 for v in rows), 'Waiting partner moved'
        if restore:
            assert restored,'No corner snapshot restore was exercised'
            assert any(a['layer']==0 and a['state'][1]==6 and a['x']>2000
                       for a in rows[-1]['actors']), 'Restored return used the waiting view'
            shots=[a for row in rows for a in row['shots'] if a['subtype'] in (2,3)
                   and a['layer']==0 and a['phase'][0]]
            assert shots,'No initialized native background shots were observed'
            assert min(a['x'] for a in shots)>2000,'Background shots used the waiting view'
        results.append(dict(leader=leader,observations=len(rows),captures=captures,
                            phases=[list(p) for p in sorted(phases)],restored=restored,observation_only=True))
    return dict(native_first_flybys=results)
