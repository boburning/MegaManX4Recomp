"""Replay checkpoint/carry boundaries in an owned, hidden X4 debug instance.

Uses native stage loaders, player controllers, withdrawal and death animations.
Run only against a private development copy with separate memory cards.
"""
import argparse
import json
from pathlib import Path
import struct
import time
from coop_debug import Runtime


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    runtime = Runtime(args.port)
    events = []

    def observe():
        raw = runtime.read(runtime.diagnostic(), 0x464)
        # The body mirror is coherent, but its PLAY copy deliberately stops
        # updating during native pause/loading. Read transition fields live.
        play = runtime.read(0x801721c0,0x64)
        bodies = (raw[0x300:0x3e4], raw[0x100:0x1e4])
        return dict(frame=struct.unpack_from('<I', raw, 4)[0], enrolled=raw[8], failed=raw[12],
                    mode=play[0], phase=play[1], stage=play[0xc], section=play[0xd],
                    checkpoint=play[0x1d], gate=play[0x1c], lives=play[0x44],
                    players=[dict(active=p[0], visible=p[3], state=p[4], action=p[5], hp=p[0x5c]&127,
                                  x=struct.unpack_from('<i',p,8)[0]/65536,
                                  y=struct.unpack_from('<i',p,12)[0]/65536) for p in bodies])

    def wait(label, predicate, timeout=30):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            result = observe()
            if result['failed']:
                raise AssertionError(result)
            if predicate(result):
                result['label'] = label
                events.append(result)
                print(json.dumps(result), flush=True)
                (args.output / 'report.json').write_text(json.dumps(events, indent=2))
                runtime.screenshot(args.output / (label + '.png'))
                return result
            time.sleep(.06)
        raise TimeoutError((label, result))

    def live(result, stage=3, section=0):
        return result['mode']==6 and result['phase']==0 and result['enrolled'] and not result['gate'] and \
               result['stage']==stage and result['section']==section and \
               all(p['active'] and p['state']==1 and p['action']==2 and p['hp']==32 for p in result['players'])

    def counter(name):
        return next((c['count'] for c in runtime.request('mod_counters')['counters'] if c['name']==name), 0)

    campaign = runtime.players()[0][2]
    assert campaign in (0,1)
    try:
        runtime.fixture(5,3)
        runtime.fixture(1,3,0,campaign)
        wait('native-section0-start', lambda s: live(s) and s['checkpoint']==0)
        runtime.fixture(3,1)
        checkpoint = wait('same-section-checkpoint-alive', lambda s: live(s) and s['checkpoint']==1)
        for seat in (0,1):
            runtime.input(0xfff7,seat); time.sleep(.15); runtime.release()
            wait('pause-seat'+str(seat), lambda s: s['mode']==6 and s['phase']==2)
            wait('pause-ready-seat'+str(seat), lambda s:
                 s['phase']==2 and runtime.read(0x801754a4,1)[0]==1)
            runtime.input(0xfff7,seat)
            wait('resume-seat'+str(seat), live)
            runtime.release()
        departures = counter('mmx4.coop.departures')
        runtime.input(0xfffe,1)
        wait('withdrawal-started', lambda s: counter('mmx4.coop.departures')>departures, timeout=10)
        runtime.release()
        wait('withdrawn', lambda s: not s['players'][1]['active'] and s['players'][1]['hp']==32)
        runtime.fixture(3,1)
        wait('withdrawn-checkpoint-carry', lambda s:
             s['mode']==6 and s['phase']==0 and not s['gate'] and s['checkpoint']==1 and
             s['players'][0]['action']==2 and not s['players'][1]['active'] and s['players'][1]['hp']==32)
        runtime.input(0xfffe,1); time.sleep(.15); runtime.release()
        rejoined = wait('rejoined-after-checkpoint', live)
        assert rejoined['lives']==checkpoint['lives']
        runtime.fixture(1,3,1,campaign)
        wait('section1-alive', lambda s: live(s,section=1))
        runtime.fixture(3,1)
        before_death = wait('section1-checkpoint-alive', lambda s: live(s,section=1) and s['checkpoint']==1)
        runtime.fixture(2,0,0x80)
        survivor = wait('P1-dead-P2-survives', lambda s:
                        s['mode']==6 and s['phase']==0 and s['players'][0]['state']==3 and
                        not s['players'][0]['hp'] and s['players'][1]['hp']==32)
        runtime.input(0xbfff,1)
        wait('survivor-native-jump', lambda s:
             s['players'][1]['hp']==32 and s['players'][1]['y']<survivor['players'][1]['y']-15,timeout=8)
        runtime.release()
        assert observe()['lives']==before_death['lives']
        runtime.fixture(2,1,0x80)
        retry = wait('native-team-retry', lambda s: live(s,section=1),timeout=45)
        assert retry['lives']==(before_death['lives']-1)&255
        print('PASS: native checkpoint, both pause owners, withdrawn carry/rejoin, section transfer, survivor input and one-life team retry',flush=True)
    finally:
        runtime.release()


if __name__=='__main__':
    main()
