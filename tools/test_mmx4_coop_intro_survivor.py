"""Native local co-op intro-act2 boss qualification for GitHub issue25.

Uses original stage initialization and observed safe pre-boss coordinates;
only controller input crosses the real event. Fatal HP setup runs the native
player death. The rendered WARNING is General24, not Scenarios' legacy
'warning' field at80141BDC (which is shared fade state). Keep complete output
for visual review. Requires a debug build and the user's original disc.
"""
from pathlib import Path
import json,os,shutil,socket,subprocess,sys,time
root=Path(__file__).resolve().parents[1]
from coop_debug import Runtime
from test_mmx4_coop_regressions import Scenarios
from test_mmx4_coop_netplay import change_config,sha256_file
import argparse
parser=argparse.ArgumentParser(description='Hidden original-disc intro boss survivor qualification; private cards, no desktop input.')
for name in ('exe','bios','disc','game','output'):parser.add_argument('--'+name,type=Path,required=True)
parser.add_argument('--campaign',type=int,choices=(0,1),default=0)
parser.add_argument('--dead-seat',type=int,choices=(0,1),default=0)
parser.add_argument('--entry',choices=('checkpoint','first','race','settled','warning'),default='settled')
args=parser.parse_args()
for name in ('exe','bios','disc','game'):setattr(args,name,getattr(args,name).resolve(strict=True))
campaign=args.campaign;dead=args.dead_seat;run=args.output
first_approach=args.entry!='checkpoint';at_warning=args.entry=='warning';death_race=args.entry=='race'
run=run.resolve();run.mkdir(parents=True,exist_ok=False)
source=args.exe.parent;exe=run/args.exe.name
shutil.copy2(source/exe.name,exe)
for name in ('assets','mods'):shutil.copytree(source/name,run/name)
for dll in source.glob('*.dll'):shutil.copy2(dll,run/dll.name)
(run/'mods/state.toml').write_text('format_version = 2\n\n[[feature]]\npackage_id = "mmx4.coop"\nid = "coop"\nenabled = true\n[feature.values]\ncameras = "unified"\nhud_layout = "stacked"\n')
cfg=args.game.read_text()
for section,key,value in [('game','exe',str(root/'mmx4/SLUS_005.61')),('video','internal_resolution','native'),('runtime','memcard_dir',str(run/'private-cards')),('runtime','overlay_autocompile_cmd','')]:cfg=change_config(cfg,section,key,value)
(run/'game.toml').write_text(cfg)
env={k:v for k,v in os.environ.items()if not k.startswith(('PSX_','RNET_','LNG_','MMX4_DIAGNOSTIC_','MMX4_RECOMP_'))and k not in ('APPIMAGE','APPDIR','RECOMP_APPIMAGE_PATH')};env['MMX4_RECOMP_DATA_DIR']=str(run);env['SDL_AUDIODRIVER']='dummy'
with socket.socket()as sock:sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
with (run/'runtime.log').open('wb')as log:
    process=subprocess.Popen([str(exe),'--no-launcher','--hidden-window','--renderer','opengl','--game',str(run/'game.toml'),'--bios',str(args.bios),'--disc',str(args.disc),'--memcard-dir',str(run/'private-cards'),'--debug-port',str(port)],cwd=run,env=env,stdout=log,stderr=log,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
record=dict(campaign=campaign,dead_seat=dead,first_approach=first_approach,executable_sha256=sha256_file(args.exe),status='running')
s=None;r=Runtime(port)
try:
    deadline=time.monotonic()+30
    while True:
        try:r.read(0x801721c0,4);break
        except OSError:assert process.poll()is None and time.monotonic()<deadline;time.sleep(.1)
    r.request('turbo',enabled=1);sleep=time.sleep;time.sleep=lambda duration:sleep(duration*.15)
    try:r.boot(campaign=campaign)
    finally:time.sleep=sleep;r.request('turbo',enabled=0)
    s=Scenarios(r,run,campaign);s.watch_enemy=8
    r.fixture(5,0 if first_approach else 3)
    s.load(0,1,None if first_approach else 2)
    if first_approach:
        r.teleport_both(3936,443)
        r.teleport(1,3960,443 if campaign else 442)
        time.sleep(.3)
    if death_race:r.teleport_both(4208,443)
    if args.entry in('settled','warning'):
        s.wait('native-camera-settled',lambda v:v['camera'][5]>3776 and all(p['grounded']for p in v['players']),15)
    if at_warning:
        r.teleport(0,3984,442 if campaign else 443)
        def warning_active(v):
            pool=r.read(0x80142f98,32*0x30)
            return any(pool[i*0x30]and pool[i*0x30+1]==24 and pool[i*0x30+5]==1 for i in range(32))
        r.input(0xffdf,0);r.input(0xffdf,1)
        try:s.wait('native-warning-live-before-death',warning_active,20)
        finally:r.release()
        r.screenshot(run/'warning-before-death.png')
    record['before_death']=s.sample('before-death',True)
    if not at_warning:assert not any(e['type']==8 and e['hp']>1 for e in s.enemies()),'Boss spawned before setup'
    r.fixture(2,dead,0x80)
    if not death_race:s.wait('original-partner-death-finished',lambda v:v['players'][dead]['state']==3 and v['players'][dead]['hp']==0,12)
    before=s.sample('survivor-before-approach',True);record['before_entry']=before
    survivor=dead^1;deadline=time.monotonic()+35;saw_warning=False;saw_boss=False;next_capture=0;captures=0;warning_phases=set()
    while time.monotonic()<deadline:
        row=s.sample('survivor-approach')
        pool=r.read(0x80142f98,32*0x30)
        row['general']=[dict(slot=i,raw=list(p[:8]))for i in range(32)if (p:=pool[i*0x30:(i+1)*0x30])[0]]
        if first_approach and time.monotonic()>next_capture and captures<20:
            r.screenshot(run/('approach-%02d.png'%captures));captures+=1;next_capture=time.monotonic()+.7
        word=[g for g in row['general']if g['raw'][1]==24]
        warning_phases.update(g['raw'][5]for g in word)
        saw_warning |= any(g['raw'][5]==1 for g in word)
        saw_boss |= any(e['type']==8 and e['hp']>1 and e['state'][0] for e in s.enemies())
        live=row['players'][survivor]
        if saw_boss and not word and not row['warning'] and not row['blocked'] and live['state']==1 and live['action']>=2 and not live['script'] and not live['door']:
            break
        if row['blocked'] and not row['warning']:
            r.input(0xbfff,survivor);time.sleep(.05);r.release()
        else:r.input(0xffdf,survivor)
        time.sleep(.04)
    else:raise TimeoutError(f'Survivor boss entry stalled: {row}')
    r.release();after=s.sample('survivor-native-boss-fight',True)
    assert after['players'][dead]['hp']==0 and after['players'][dead]['state']==3,'Boss entry revived corpse'
    assert after['players'][survivor]['hp']>0 and after['lives']==before['lives'],after
    r.input(0xffdf,survivor);time.sleep(.3);r.release()
    moved=s.sample('survivor-controls-after-warning',True)
    assert moved['players'][survivor]['x']>after['players'][survivor]['x']+5,'Survivor controls stayed locked'
    record.update(status='passed',saw_warning=saw_warning,warning_phases=sorted(warning_phases),saw_native_boss=saw_boss,after_entry=after,after_controls=moved)
except Exception as error:
    record.update(status='failed',error=f'{type(error).__name__}: {error}')
    if s:
        try:record['failed_state']=s.sample('failed-state',True)
        except Exception as sample_error:record['sample_error']=str(sample_error)
finally:
    if s:(run/'events.json').write_text(json.dumps(s.events,indent=2))
    if process.poll()is None:
        try:r.release();r.request('quit')
        except (RuntimeError,OSError):pass
        try:process.wait(timeout=10)
        except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=10)
    record['returncode']=process.returncode
    (run/'report.json').write_text(json.dumps(record,indent=2))
print(json.dumps(record),flush=True)
raise SystemExit(0 if record['status']=='passed'else 1)
