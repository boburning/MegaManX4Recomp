"""Measure real, paced Split peers using native input and private game data.

Restoration hashing is off by default to match human playtest settings. All
sandbox/device guards and canonical peer digest checks remain active.
"""
import argparse
import ctypes
import json
import os
from pathlib import Path
import time
import traceback

from test_mmx4_coop_netplay import (Exercise, change_config, prepare_peer,
                                   require, reserve_ports, sha256_file)
from test_mmx4_coop_lobby import require_hidden


def process_seconds(process):
    if os.name != 'nt':
        return None
    class FileTime(ctypes.Structure):
        _fields_ = [('low', ctypes.c_uint32), ('high', ctypes.c_uint32)]
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.restype = ctypes.c_void_p
    kernel.GetProcessTimes.argtypes = [ctypes.c_void_p] + [ctypes.POINTER(FileTime)] * 4
    kernel.CloseHandle.argtypes = [ctypes.c_void_p]
    handle = kernel.OpenProcess(0x1000, False, process.pid)
    require(bool(handle), 'Cannot query owned peer CPU time')
    try:
        values = [FileTime() for _ in range(4)]
        require(kernel.GetProcessTimes(handle, *[ctypes.byref(v) for v in values]),
                'Owned peer CPU query failed')
        return sum((v.high << 32) | v.low for v in values[2:]) * 1e-7
    finally:
        kernel.CloseHandle(handle)


class PerformanceExercise(Exercise):
    def snapshot(self, peer):
        return dict(time=time.perf_counter(), cpu=process_seconds(peer.process),
                    state=peer.observe(), status=peer.status(),
                    frame=peer.request('frame')['frame'],
                    stats=peer.request('render_pass_stats'))

    def sample(self, label):
        self.both('input', 0)
        self.wait_ticks(15)
        before = [self.snapshot(peer) for peer in self.peers]
        time.sleep(self.args.window)
        after = [self.snapshot(peer) for peer in self.peers]
        rows = []
        self.record['samples'].append(dict(name=label,peers=rows))
        for peer, a, b in zip(self.peers, before, after):
            seconds = b['time'] - a['time']
            rates = dict(vblank_hz=(b['frame'] - a['frame']) / seconds,
                         net_hz=(b['status']['tick'] - a['status']['tick']) / seconds,
                         world_hz=(b['state']['frames'] - a['state']['frames']) / seconds,
                         draws_hz=(b['stats']['local_views'] - a['stats']['local_views']) / seconds)
            if a['cpu'] is not None:
                rates['cpu_percent'] = (b['cpu'] - a['cpu']) / seconds * 100
            rows.append(dict(seat=peer.seat, seconds=seconds, rates=rates, before=a, after=b,
                             frame_perf=peer.request('frame_perf'),
                             phase=peer.request('phase_profile', window=5)))
        for row in rows:
            a,b=row['before'],row['after']
            require(all(b['state'][s]['hp'] and b['state'][s]['state'] == 1 for s in ('p1', 'p2')),
                    'A participant died; performance window is invalid')
            require(b['stats']['local_views'] > a['stats']['local_views'],
                    'Independent presentation stopped')
        print(label, json.dumps([dict(seat=r['seat'], rates=r['rates'],
                                     pass_ms=r['after']['stats']['avg_pass_ms']) for r in rows]), flush=True)

    def split_world_observation(self, label):
        result = super().split_world_observation(label)
        if label in ('split-before-separation', 'split-separated', 'split-rejoined'):
            self.sample(label)
        return result


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--window', type=float, default=6)
    parser.add_argument('--verify', type=int, choices=(0, 1), default=0)
    parser.add_argument('--invulnerable',action='store_true',
                        help='Synchronized development fixture; requires a debug netplay build')
    args = parser.parse_args()
    require(args.window >= 2, 'Use at least two seconds per sample')
    args.exe = args.exe.resolve(); args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=False)
    args.game_root=root; args.config=root/'game.toml'
    args.disc=root/'mmx4/Mega Man X4.cue'; args.bios=root/'build-github25-debug/bios/openbios.bin'
    args.cameras='split'; args.frontend='hidden-window'
    args.delay=2; args.latency=args.jitter=0; args.seed=1296914484
    args.boot_timeout=180; args.phase_timeout=45
    ports, reservations = reserve_ports()
    # One shared session identity, not a separately evaluated value per peer.
    session=time.time_ns()&0x7fffffff
    peers=[prepare_peer(args,args.out,seat,ports,False,session) for seat in (0,1)]
    for peer in peers:
        peer.environment.update(SDL_AUDIODRIVER='dummy',PSX_RENDER_PASS_VERIFY=str(args.verify))
        config=(peer.directory/'game.toml').read_text()
        for section,key,value in (('runtime','bios_hle',True),('video','internal_resolution','1080p'),
                                  ('video','aspect_ratio','4:3'),('controller','p1_device','none'),
                                  ('controller','p2_device','none')):
            config=change_config(config,section,key,value)
        (peer.directory/'game.toml').write_text(config)
    report=dict(success=False,verify=args.verify,invulnerable=args.invulnerable,executable=str(args.exe),
                executable_sha256=sha256_file(args.exe),boot_modes=[],checkpoints=[],observations=[],samples=[])
    exercise=PerformanceExercise(args,peers,report)
    try:
        for reservation in reservations: reservation.close()
        for peer in peers: peer.launch()
        report['pids']=[peer.process.pid for peer in peers]
        for peer in peers: require_hidden(peer.process.pid)
        exercise.boot(0)
        if args.invulnerable:
            from coop_netplay_debug import NetplayDevelopment
            NetplayDevelopment(*ports[2:]).fixture(5,3)
            for peer in peers:
                overlay=peer.request('overlay_status')
                if overlay['save_menu_open']:
                    peer.input(0x2000);exercise.wait_ticks(3);peer.input(0);exercise.wait_ticks(6)
                require(not peer.request('overlay_status')['save_menu_open'],
                        'Development packet left a host menu open')
        exercise.split_separation_probe()
        report['success']=True
    except BaseException as error:
        report.update(error=str(error),traceback=traceback.format_exc())
        raise
    finally:
        for reservation in reservations: reservation.close()
        for peer in peers: peer.stop()
        exercise.pool.shutdown()
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
