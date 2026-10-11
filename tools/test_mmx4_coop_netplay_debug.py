"""Verify synchronized cheats and native door handoffs in two hidden peers.

Both peers must use the separate MMX4_COOP_NETPLAY_DEBUG build. Private saves,
loopback ports and owned PIDs follow the normal netplay harness. No RAM writes.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import time
from coop_netplay_debug import NetplayDevelopment
from test_mmx4_coop_netplay import Exercise, prepare_peer, reserve_ports, change_config, ValidationError


def tolerate_loading_reads(peer):
    """Read-only TCP requests may time out while native linking/loading owns
    the emulation thread. Keep actual exit/desync failures and mutation errors
    immediate; never retry or relax a simulation command or watchdog policy.
    """
    original=peer._request
    def request(cmd, *, allow_error=False, **fields):
        deadline=time.monotonic()+12
        while True:
            try:
                return original(cmd,allow_error=allow_error,**fields)
            except ValidationError as error:
                if cmd not in ('netplay_status','read_ram','mod_counters') or \
                        'emu busy or frozen' not in str(error) or \
                        not peer.running() or time.monotonic()>=deadline:
                    raise
                time.sleep(.05)
    peer._request=request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--config', type=Path, default=Path('game.toml'))
    parser.add_argument('--game-root', type=Path, default=Path.cwd())
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cameras', choices=('unified', 'split'), default='unified')
    parser.add_argument('--campaign', type=int, choices=(0, 1), default=0)
    parser.add_argument('--doors', action='store_true', help='Also traverse the P2-led Cyber door')
    parser.add_argument('--train-only',action='store_true',help='Qualify both Slash Split background panes and CRCs')
    parser.add_argument('--latch-only',action='store_true',help='Qualify the native Mushroom latch and matching peer state')
    parser.set_defaults(latency=0, jitter=0, seed=714, delay=2,
                        frontend='headless-opengl', boot_timeout=100, phase_timeout=35)
    args = parser.parse_args()
    for name in ('exe', 'bios', 'disc', 'config', 'game_root', 'output'):
        setattr(args, name, getattr(args, name).resolve())
    args.output.mkdir(parents=True, exist_ok=False)
    ports, reservations = reserve_ports()
    report = dict(status='running', development=True, ports=ports, campaign=args.campaign,
                  cameras=args.cameras, boot_modes=[], checkpoints=[], observations=[], controls=[])
    peers = []
    exercise = None
    try:
        session = int(time.time()) & 0xFFFFFFFF
        peers = [prepare_peer(args, args.output, seat, ports, False, session)
                 for seat in (0, 1)]
        for peer in peers:
            tolerate_loading_reads(peer)
            peer.environment['PATH'] = r'C:\msys64\mingw64\bin;' + os.environ.get('PATH', '')
            peer.environment['SDL_AUDIODRIVER'] = 'dummy'
            config=peer.directory/'game.toml'
            config.write_text(change_config(config.read_text(),'video','internal_resolution','native'))
        for reservation in reservations:
            reservation.close()
        reservations = []
        for peer in peers:
            peer.launch()
        report['pids'] = [peer.process.pid for peer in peers]
        exercise = Exercise(args, peers, report)
        exercise.boot(args.campaign)
        debug = NetplayDevelopment(ports[2], ports[3])
        if args.latch_only:
            from mmx4_netplay_runtime import NetplayRuntime
            runtime=NetplayRuntime(debug)
            debug.fixture(5,3)
            runtime.fixture(1,3,1,args.campaign)
            report['stage_skip_overlays']=[peer.request('overlay_status') for peer in peers]
            for seat,overlay in enumerate(report['stage_skip_overlays']):
                if overlay['save_menu_open']:
                    print('Development packet opened the host save menu on peer',seat,flush=True)
                    runtime.input(0xdfff,seat)
                    time.sleep(.15)
            runtime.release()
            runtime.wait_playable(45,advance_dialogue=True)
            runtime.fixture(3,3)
            runtime.wait_playable(45,advance_dialogue=True)
            def latch(label):
                observations=[]
                for seat,peer in enumerate(peers):
                    pool=peer.read(0x80165A30,32*0x8c)
                    found=[]
                    for index in range(32):
                        actor=pool[index*0x8c:(index+1)*0x8c]
                        if actor[0] and actor[1]==13 and struct.unpack_from('<h',actor,14)[0]==488:
                            found.append(dict(state=actor[4],action=actor[5]))
                    if len(found)!=1:raise AssertionError((label,seat,found))
                    observations.append(found[0])
                    debug.peers[seat].screenshot(args.output/(label+'-peer'+str(seat)+'.png'))
                report['observations'].append(dict(label=label,latches=observations))
                return observations
            runtime.teleport(1,6114,592)
            runtime.teleport(0,6152,452)
            exercise.wait_ticks(6)
            waiting=latch('partner-below')
            if any(p!={'state':1,'action':0} for p in waiting):
                raise AssertionError(('Latch closed over the visible partner',waiting))
            tick=min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('Mushroom visible-partner wait CRC',tick,min_digests=2)
            runtime.teleport_both(6152,452)
            exercise.wait_ticks(32)
            closed=latch('both-clear')
            if any(p['state']!=2 and p['action']!=1 for p in closed):
                raise AssertionError(('Latch failed to close after both clear',closed))
            tick=min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('Mushroom native closure CRC',tick,min_digests=2)
            report.update(status='passed',native_latch=True)
            print('Netplay: native latch waits for the visible partner and closes with matching peer CRCs',flush=True)
            return
        if args.train_only:
            if args.cameras!='split':raise ValueError('Train scenery qualification requires --cameras split')
            from mmx4_netplay_runtime import NetplayRuntime
            from mmx4_web_probe import Probe
            runtime=NetplayRuntime(debug)
            output=args.output/'train-background';output.mkdir()
            probe=Probe(runtime,output,args.campaign)
            trace_targets=(0x800D8648,0x800D9218,0x800D964C,0x80021A20,0x8002196C)
            for target in trace_targets:peers[0].request('fntrace_arm',target=hex(target))
            peers[0].request('wtrace_arm',lo='0x801419B0',hi='0x80141AAC')
            try:
                for owner in (1,0):
                    probe.train_background(owner,2)
                    captures=[]
                    for frame in (0,1):
                        for peer in peers:
                            shot=output/f'train-owner-{owner}-peer-{peer.seat}-frame-{frame}.png'
                            peer.request('screenshot_hires',path=str(shot))
                            captures.append(str(shot))
                        time.sleep(.2)
                    probe.cases[-1]['both_peer_captures']=captures
                    tick=min(peer.status()['tick'] for peer in peers)
                    exercise.checkpoint('native Slash backgrounds owner '+str(owner),tick,min_digests=2)
            finally:
                runtime.release()
                traces={hex(target):peers[0].request('fntrace_dump',
                    target_lo=hex(target),target_hi=hex(target+4),count=128)
                    for target in trace_targets}
                traces['camera_writes']=peers[0].request('wtrace_dump',count=128,newest=1,
                    addr_lo='0x80141A04',addr_hi='0x80141AAC')
                (output/'native-traces.json').write_text(json.dumps(traces,indent=2))
                (output/'stage-interactions.json').write_text(json.dumps(
                    dict(cases=probe.cases,events=probe.events),indent=2))
            report.update(status='passed',train_background_owners=2)
            print('Split netplay: both native Slash backgrounds advance with matching peer CRCs',flush=True)
            return
        debug.fixture(5, 3)
        debug.teleport(1, 160, 136)
        exercise.wait_ticks(12)
        exercise.wait_observation('P2 synchronized teleport', lambda s:
                                  150 <= s['p2']['x'] <= 170 and s['p2']['hp'] > 0)
        tick = min(peer.status()['tick'] for peer in peers)
        exercise.checkpoint('teleport/invulnerability CRC', tick, min_digests=2)
        debug.fixture(1, 6, 1, args.campaign)
        exercise.wait_observation('Cyber stage loaded', lambda s:
                                  s['stage'] == 6 and s['section'] == 1 and s['enrolled'] and
                                  s['mode'] == 6 and s['minor'] == 0 and
                                  not s['pause_suppressed'] and s['p2']['hp'] > 0 and
                                  all(s[p]['grounded'] and s[p]['action'] == 2 for p in ('p1', 'p2')))
        debug.fixture(3, 1)
        exercise.wait_observation('Cyber checkpoint loaded', lambda s:
                                  s['stage'] == 6 and s['enrolled'] and s['p1']['y'] > 2400 and
                                  not s['pause_suppressed'] and s['p2']['hp'] > 0)
        tick = min(peer.status()['tick'] for peer in peers)
        exercise.checkpoint('native load/checkpoint CRC', tick, min_digests=2)
        debug.fixture(2, 1, 24)
        exercise.wait_observation('P2 synchronized health', lambda s: s['p2']['hp'] == 24)
        debug.fixture(2, 1, 32)
        if args.doors:
            # Keep both inside Unified's 240-pixel separation bound. Door tile
            # centers are solid; use the observed native floor before the door.
            debug.teleport(0, 568, 2507 if args.campaign == 0 else 2506)
            debug.teleport(1, 600, 2506 if args.campaign == 0 else 2507)
            peers[1].input(0x20)
            try:
                exercise.wait_observation('P2 native Cyber door entered', lambda s:
                                          s['p2']['door'] != 0)
            finally:
                peers[1].input(0)
            exercise.wait_observation('P2 native Cyber door returned', lambda s:
                                      all(s[p]['active'] and s[p]['hp'] > 0 and
                                          s[p]['state'] == 1 and s[p]['grounded']
                                          for p in ('p1', 'p2')) and not s['pause_suppressed'])
            tick = min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('P2 door/handoff CRC', tick, min_digests=2)
        debug.fixture(1, 5, 0, args.campaign)
        exercise.wait_observation('Jet riders enrolled', lambda s:
                                  s['mode'] == 6 and s['minor'] == 0 and s['stage'] == 5 and s['enrolled'] and
                                  all(s[p]['active'] and s[p]['hp'] > 0 and
                                      s[p]['vehicle'] == 255 for p in ('p1', 'p2')))
        counts = []
        for peer in peers:
            actors = peer.read(0x80142F98, 32 * 0x30)
            counts.append(sum(actors[i * 0x30] != 0 and actors[i * 0x30 + 1] == 0x1B
                              for i in range(32)))
        report['jet_ready_actors'] = counts
        shared = []
        for peer in peers:
            counters = peer.request('mod_counters')['counters']
            shared.append(sum(c['count'] for c in counters if c['name'] == 'mmx4.coop.shared-ride-ready'))
        report['jet_shared_ready_callbacks'] = shared
        if any(count > 1 for count in counts) or not all(shared):
            raise AssertionError(f'Expected one shared READY presentation per peer: {counts}/{shared}')
        tick = min(peer.status()['tick'] for peer in peers)
        exercise.checkpoint('Jet shared Ready CRC', tick, min_digests=2)
        report['status'] = 'passed'
        print('Development netplay cheats: both peers agree after teleport and native loads', flush=True)
    except BaseException as error:
        report.update(status='failed', error=f'{type(error).__name__}: {error}')
        print(report['error'], flush=True)
        for peer in peers:
            try:
                report.setdefault('failure_states', []).append(peer.observe())
                report.setdefault('failure_overlays', []).append(peer.request('overlay_status'))
            except Exception:
                pass
        raise
    finally:
        if exercise:
            exercise.pool.shutdown(wait=True)
        for peer in peers:
            peer.stop()
        for reservation in reservations:
            reservation.close()
        (args.output / 'report.json').write_text(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
