"""Record bounded, original-disc Frost combat for visual hit-registration QA.

Requires an existing development executable with TCP debug tools. Copies its
assets/mods into a new private output directory; launches hidden and muted.
Checkpoint, skills, health protection and positions are setup fixtures. Once a
trial starts, only controller input and read-only observations are used.

Rows and screenshots are asynchronous observations, not an atomic frame trace.
Source input excerpts transplant cadence into a fresh fight; they are not a
deterministic replay of the source session. Inspect report.json and the images;
this probe deliberately does not turn damage totals into a gameplay PASS.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

from coop_debug import Runtime
from test_mmx4_coop_lobby import require_hidden
from test_mmx4_coop_netplay import change_config
from test_mmx4_coop_regressions import Scenarios


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-dir', type=Path, required=True,
                        help='Private build containing executable, assets, mods and game.toml')
    parser.add_argument('--output', type=Path, required=True, help='New private directory')
    parser.add_argument('--executable', type=Path, help='Override runtime-dir executable')
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--campaign', type=int, choices=(0, 1), default=0)
    parser.add_argument('--mode', choices=('separate', 'shared', 'off'), default='separate')
    parser.add_argument('--cases', nargs='+', choices=('single', 'combo', 'air', 'overlap', 'fire', 'source'),
                        default=['single', 'combo', 'air', 'overlap'])
    parser.add_argument('--port', type=int, default=17890)
    parser.add_argument('--frames', type=int, default=600)
    parser.add_argument('--capture-step', type=int, default=8, help='Minimum game frames between captures')
    parser.add_argument('--basic-saber', action='store_true', help='Leave Zero without acquired skills')
    parser.add_argument('--side', type=int, choices=(-1, 1), default=-1)
    parser.add_argument('--spacing', type=int, default=40)
    parser.add_argument('--vulnerable', action='store_true',
                        help='Stop refreshing setup protection and place idle X in contact range')
    parser.add_argument('--source-inputs', type=Path)
    parser.add_argument('--source-start', type=int, default=5399)
    parser.add_argument('--source-end', type=int, default=5631)
    args = parser.parse_args()
    if not 1 <= args.frames <= 3600:
        parser.error('--frames must be between 1 and 3600')
    if not 1 <= args.capture_step <= 60:
        parser.error('--capture-step must be between 1 and 60')
    if 'source' in args.cases and not args.source_inputs:
        parser.error('source case requires --source-inputs')
    if args.source_end < args.source_start:
        parser.error('--source-end must follow --source-start')
    return args


def prepare(args):
    source, out = args.runtime_dir.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    for name in ('assets', 'mods'):
        shutil.copytree(source / name, out / name)
    shutil.copy2(args.executable.resolve() if args.executable else source / 'MegaManX4Recomp.exe',
                 out / 'MegaManX4Recomp.exe')
    (out / 'saves').mkdir()
    state_path = out / 'mods/state.toml'
    state = state_path.read_text()
    # Modify only the co-op feature block; reject an unfamiliar fixture.
    blocks = state.split('[[feature]]')
    selected = [i for i, block in enumerate(blocks)
                if re.search(r'(?m)^package_id\s*=\s*"mmx4.coop"\s*$', block)]
    if len(selected) != 1:
        raise ValueError('Expected exactly one co-op feature in private mods/state.toml')
    i = selected[0]
    block = re.sub(r'(?m)^boss_(?:scaling|cooldowns)\s*=.*\n?', '', blocks[i])
    block = re.sub(r'(?m)^enabled\s*=.*$', 'enabled = true', block)
    if '[feature.values]' not in block:
        block += '\n[feature.values]\n'
    values = '\nboss_scaling = "' + ('false' if args.mode == 'off' else 'true') + '"'
    values += '\nboss_cooldowns = "' + ('shared' if args.mode == 'shared' else 'per_player') + '"'
    blocks[i] = block.replace('[feature.values]', '[feature.values]' + values, 1)
    state_path.write_text('[[feature]]'.join(blocks))
    config = (source / 'game.toml').read_text()
    config = change_config(config, 'runtime', 'memcard_dir', str(out / 'saves'))
    config = change_config(config, 'game', 'disc', str(args.disc.resolve()))
    (out / 'game.toml').write_text(config)
    return out


def source_buttons(args):
    if not args.source_inputs:
        return {}
    rows = {}
    for line in args.source_inputs.read_text().splitlines():
        if line.startswith('N '):
            row = [int(v) for v in line[2:].split(',')]
            if args.source_start <= row[0] <= args.source_end:
                if row[0] in rows:
                    raise ValueError(f'Duplicate source frame {row[0]}')
                rows[row[0]] = row[3]  # Transplant the source P2 controller.
    if set(rows) != set(range(args.source_start, args.source_end + 1)):
        raise ValueError('Source excerpt has missing or duplicate frame coverage')
    return rows


def main():
    args = arguments()
    excerpt = source_buttons(args)
    out = prepare(args)
    runtime = Runtime(args.port)
    scenarios = Scenarios(runtime, out, args.campaign)
    zero, xseat = args.campaign ^ 1, args.campaign
    report = {'args': {k: str(v) if isinstance(v, Path) else v for k, v in vars(args).items()},
              'executable_sha256': hashlib.sha256((out / 'MegaManX4Recomp.exe').read_bytes()).hexdigest(),
              'trials': []}
    env = {k: v for k, v in os.environ.items() if not k.startswith(('PSX_', 'RNET_', 'LNG_', 'MMX4_'))}
    env.update(MMX4_RECOMP_DATA_DIR=str(out), SDL_AUDIODRIVER='dummy',
               PSX_INTERNAL_RESOLUTION='native', PSX_DYNRES='0', PSX_SUPERSAMPLE='1.0')
    if os.name == 'nt':
        env['PATH'] = 'C:/msys64/mingw64/bin;' + env.get('PATH', '')

    def counters():
        return {c['name']: c['count'] for c in runtime.request('mod_counters')['counters']}

    def sample():
        diagnostic = runtime.read(diag, 0x580)
        enemy = runtime.read(actor, 0x9c)
        bodies = [diagnostic[0x300:0x3e4], diagnostic[0x100:0x1e4]]
        shots = runtime.read(0x801406f8, 16 * 0x9c) if zero == 0 else diagnostic[0x480:0x500]
        stride = 0x9c if zero == 0 else 8
        return {'frame': struct.unpack_from('<I', diagnostic, 4)[0],
                'hp': enemy[0x5c], 'timer': enemy[0x94], 'boss': list(enemy[4:7]),
                'boss_x': struct.unpack_from('<i', enemy, 8)[0] / 65536,
                'boss_y': struct.unpack_from('<i', enemy, 12)[0] / 65536,
                'table': hex(struct.unpack_from('<I', enemy, 0x58)[0]), 'token': enemy[0x65],
                'classes': [shots[k + 1] for k in range(0, len(shots), stride) if shots[k]],
                'players': [{'x': struct.unpack_from('<i', b, 8)[0] / 65536,
                             'y': struct.unpack_from('<i', b, 12)[0] / 65536,
                             'hp': b[0x5c] & 127, 'state': list(b[4:7]),
                             'hitpause': b[0xbc], 'body': b.hex()} for b in bodies]}

    def face(row, seat, force=False):
        dx = row['boss_x'] - row['players'][seat]['x']
        return (0xffdf if dx > 0 else 0xff7f) if force or abs(dx) > 45 else 0xffff

    command = [str(out / 'MegaManX4Recomp.exe'), '--hidden-window', '--no-launcher',
               '--game', str(out / 'game.toml'), '--bios', str(args.bios.resolve()),
               '--disc', str(args.disc.resolve()), '--memcard-dir', str(out / 'saves'),
               '--debug-port', str(args.port)]
    with (out / 'runtime.log').open('wb') as log:
        process = subprocess.Popen(command, cwd=out, env=env, stdout=log, stderr=log,
                                   creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        try:
            deadline = time.monotonic() + 40
            while time.monotonic() < deadline:
                try:
                    runtime.request('get_registers')
                    break
                except OSError:
                    if process.poll() is not None:
                        raise RuntimeError('Private game exited during boot')
                    time.sleep(.1)
            else:
                raise TimeoutError('Debug server readiness')
            require_hidden(process.pid)
            runtime.boot(campaign=args.campaign)
            diag = runtime.diagnostic()
            for case in args.cases:
                runtime.release()
                runtime.fixture(5, 3)
                scenarios.load(12, 0, 9)
                runtime.fixture(5, 3)
                runtime.fixture(7, zero, 0, 0 if args.basic_saber else 1)
                deadline = time.monotonic() + 60
                while time.monotonic() < deadline:
                    state = scenarios.sample('admission')
                    boss = next((e for e in scenarios.enemies() if e['type'] == 57 and e['hp'] == 48), None)
                    if (boss and boss['state'][0] == 1 and not state['blocked'] and not state['warning']
                            and all(p['state'] == 1 and not p['script'] for p in state['players'])):
                        break
                    for seat in (0, 1):
                        runtime.input(0xbfff if state['blocked'] or state['warning'] else 0xffdf, seat)
                    time.sleep(.07)
                    runtime.release()
                else:
                    raise TimeoutError('Frost admission')
                actor = 0x8013bed0 + boss['slot'] * 0x9c
                floor = int(max(p['y'] for p in state['players']))
                runtime.teleport(zero, int(boss['x']) + args.side * args.spacing, floor)
                runtime.teleport(xseat, int(boss['x']) - (20 if args.vulnerable else 100), floor)
                runtime.input(0xffdf if args.side < 0 else 0xff7f, zero)
                runtime.input(0xffdf, xseat)
                time.sleep(.035)
                runtime.release()
                time.sleep(.08)
                if args.vulnerable:
                    runtime.fixture(5, 0)  # Existing setup immunity expires naturally.
                runtime.request('wtrace_range', lo=hex(actor + 0x58), hi=hex(actor + 0x9c))
                runtime.request('wtrace_clear')
                trial = {'case': case, 'before': counters(), 'rows': []}
                report['trials'].append(trial)
                target = out / case
                target.mkdir()
                startframe = sample()['frame']
                nextattack, downuntil, jumpuntil, airattack = startframe, 0, 0, 0
                lastpad, lastshot, capture_index = [None, None], -100, 0
                deadline = time.monotonic() + max(90, args.frames / 4)
                limit = args.source_end - args.source_start + 1 if case == 'source' else args.frames
                print('READY', case, args.mode, args.campaign, startframe, flush=True)
                while time.monotonic() < deadline:
                    row = sample()
                    frame = row['frame']
                    elapsed = frame - startframe
                    row['seconds'] = elapsed / 60
                    if elapsed >= limit:
                        trial['complete'] = True
                        break
                    if row['hp'] == 0 or any(p['hp'] == 0 or p['state'][0] != 1 for p in row['players']):
                        trial['early_end'] = 'boss or hero left active combat'
                        break
                    starting = frame >= nextattack
                    if starting:
                        nextattack = frame + (40 if case == 'single' else 9 if case == 'combo' else 70 if case == 'fire' else 38)
                        if case in ('air', 'overlap'):
                            jumpuntil, airattack = frame + 4, frame + 7
                        else:
                            downuntil = frame + 4
                    if airattack and frame >= airattack:
                        downuntil, airattack = frame + 4, 0
                    buttons = face(row, zero, starting)
                    if frame < downuntil:
                        buttons &= 0xefef if case == 'fire' else 0x7fff
                    if case in ('air', 'overlap') and frame < jumpuntil:
                        buttons &= 0xbfff
                    if case == 'source':
                        row['source_frame'] = args.source_start + elapsed
                        buttons = excerpt[row['source_frame']]
                    if buttons != lastpad[zero]:
                        runtime.input(buttons, zero)
                        lastpad[zero] = buttons
                    if case == 'overlap':
                        other = face(row, xseat)
                        if downuntil - 4 <= frame < downuntil + 5:
                            other &= 0x7fff
                        if other != lastpad[xseat]:
                            runtime.input(other, xseat)
                            lastpad[xseat] = other
                    row['pad_zero'] = hex(buttons)
                    trial['rows'].append(row)
                    if frame - lastshot >= args.capture_step:
                        shot = target / f'{capture_index:04d}-f{frame}-hp{row["hp"]}.png'
                        runtime.request('present_shot', path=str(shot))
                        row['shot'] = shot.name
                        lastshot = frame
                        capture_index += 1
                    time.sleep(.008)
                else:
                    trial['early_end'] = 'wall-clock limit'
                runtime.release()
                trial['after'], trial['last'] = counters(), sample()
                trial['hp_trace'] = runtime.request('wtrace_dump', count=2048,
                                                    addr_lo=hex(actor + 0x5c), addr_hi=hex(actor + 0x5d))
                # present_shot queues a host presentation capture. At dense
                # sampling a later request can replace an unpresented one.
                trial['missing_captures'] = []
                for observed in trial['rows']:
                    if observed.get('shot') and not (target / observed['shot']).is_file():
                        trial['missing_captures'].append(observed.pop('shot'))
                print('RESULT', case, 'HP', trial['last']['hp'], 'complete', trial.get('complete', False), flush=True)
                (out / 'report.json').write_text(json.dumps(report, indent=2))
        except BaseException as error:
            report['error'] = repr(error)
            raise
        finally:
            report['exit_before_cleanup'] = process.poll()
            (out / 'report.json').write_text(json.dumps(report, indent=2))
            if process.poll() is None:
                try:
                    runtime.release()
                    runtime.request('quit')
                    process.wait(timeout=15)
                except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired):
                    if process.poll() is None:
                        process.terminate()
                    process.wait(timeout=15)


if __name__ == '__main__':
    main()
