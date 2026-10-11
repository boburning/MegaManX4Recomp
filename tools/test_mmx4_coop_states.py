"""Hidden, isolated native co-op save/load, cold-load and rewind qualification."""
import argparse,json,os,shutil,socket,struct,subprocess,time
from pathlib import Path
from coop_debug import Runtime
from test_mmx4_coop_netplay import change_config

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','game','bios','disc','output'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--campaign',type=int,choices=(0,1),default=0)
    p.add_argument('--ultimate-armor',action='store_true')
    p.add_argument('--black-zero',action='store_true')
    args=p.parse_args();qa=args.output.resolve();qa.mkdir(parents=True,exist_ok=False)
    original=args.exe.resolve(strict=True);exe=qa/original.name
    shutil.copy2(original,exe)
    for name in ('assets','mods'):shutil.copytree(original.parent/name,qa/name)
    for dll in original.parent.glob('*.dll'):shutil.copy2(dll,qa/dll.name)
    (qa/'mods/state.toml').write_text('format_version = 2\n\n[[feature]]\npackage_id = "mmx4.coop"\nid = "coop"\nenabled = true\n[feature.values]\ncameras = "unified"\nhud_layout = "stacked"\n'
        f'ultimate_armor_x = {str(args.ultimate_armor).lower()}\nblack_zero = {str(args.black_zero).lower()}\n')
    (qa/'settings.toml').write_text('[video]\nrewind = true\nrewind_depth = 50\nrewind_interval = 15\n')
    cfg=args.game.resolve().read_text()
    for section,key,value in [('game','exe',str(Path(__file__).resolve().parents[1]/'mmx4/SLUS_005.61')),('video','internal_resolution','native'),('runtime','memcard_dir',str(qa/'private-cards')),('runtime','overlay_autocompile_cmd','')]:cfg=change_config(cfg,section,key,value)
    (qa/'game.toml').write_text(cfg)
    base_env={k:v for k,v in os.environ.items()if not k.startswith(('PSX_','RNET_','LNG_','MMX4_DIAGNOSTIC_','MMX4_RECOMP_'))and k not in ('APPIMAGE','APPDIR','RECOMP_APPIMAGE_PATH')}
    base_env['MMX4_RECOMP_DATA_DIR']=str(qa)
    base_env.update(PSX_REWIND='1',PSX_REWIND_DEPTH='50',PSX_REWIND_INTERVAL='15')
    events=[];process=None
    def launch(cold=False):
        nonlocal process
        with socket.socket()as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
        env=base_env.copy()
        if cold:env['PSX_LOAD_SLOT']='0'
        with (qa/('cold-runtime.log'if cold else'runtime.log')).open('wb')as log:
            process=subprocess.Popen([str(exe),'--no-launcher','--hidden-window','--renderer','opengl','--game',str(qa/'game.toml'),'--bios',str(args.bios.resolve()),'--disc',str(args.disc.resolve()),'--memcard-dir',str(qa/'private-cards'),'--debug-port',str(port)],cwd=qa,env=env,stdout=log,stderr=log,creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
        (qa/'owned.json').write_text(json.dumps(dict(pid=process.pid,port=port,cold=cold)))
        r=Runtime(port);deadline=time.monotonic()+45
        while True:
            try:r.read(0x801721c0,4);return r
            except OSError:
                assert process.poll()is None and time.monotonic()<deadline,'Runtime unavailable';time.sleep(.1)
    def stop(r=None):
        nonlocal process
        if process and process.poll()is None:
            if r:
                try:r.request('quit')
                except (OSError,RuntimeError):pass
                process.wait(timeout=15)
                assert process.returncode==0,('normal exit failed',process.returncode)
            else:process.terminate();process.wait(timeout=15)
    def observe(r,label):
        bs=r.players();event=dict(label=label,players=[dict(character=b[2],active=b[0],hp=b[0x5c]&127,state=b[4],action=b[5],x=struct.unpack_from('<i',b,8)[0]/65536,y=struct.unpack_from('<i',b,12)[0]/65536)for b in bs],save=r.request('savestate_status'),rewind=r.request('rewind_status'))
        event['canonical_code']=r.read(0x801721f7,1)[0]
        event['armor']=[dict(parts=b[0xa7],kind=b[0xb8])for b in bs]
        events.append(event);(qa/'report.json').write_text(json.dumps(events,indent=2));print(json.dumps(event),flush=True);return event
    def host(r,buttons,duration=.15):
        r.request('set_input',layer='host',buttons=hex(buttons));time.sleep(duration)
        r.request('set_input',layer='host',buttons='0xffff');time.sleep(.15)
    def until(r,command,key,expected,timeout=15):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            v=r.request(command)
            if v[key]==expected:return v
            time.sleep(.05)
        raise TimeoutError((command,key,expected,v))
    def state_operation(r,operation):
        initial=r.request('savestate_status')['generation']
        host(r,0xf7fe);until(r,'overlay_status','save_menu_open',True)
        menu=r.request('overlay_status');assert not menu['save_unavailable'],menu
        while menu['save_slot']!=0:
            host(r,0xff7f);menu=r.request('overlay_status')
        host(r,0x7fff if operation=='save' else 0xbfff)
        deadline=time.monotonic()+20
        while time.monotonic()<deadline:
            state=r.request('savestate_status')
            if state['generation']>initial and not state['pending']:
                assert state['last_ok']and state['last_op']==operation,state
                until(r,'overlay_status','save_menu_open',False);host(r,0xffff,.25);return
            time.sleep(.05)
        raise TimeoutError((operation,state))
    try:
        r=launch();r.request('turbo',enabled=1);sleep=time.sleep;time.sleep=lambda t:sleep(t*.15)
        try:r.boot(campaign=args.campaign)
        finally:time.sleep=sleep;r.request('turbo',enabled=0)
        r.wait_playable(60,advance_dialogue=True);r.fixture(5,3)
        time.sleep(.5);initial=observe(r,'initial')
        assert [b['character']for b in initial['players']]==[args.campaign,args.campaign^1]
        state_operation(r,'save');saved=observe(r,'saved')
        files=list((qa/'private-cards').rglob('*slot00.pst'));assert len(files)==1 and files[0].stat().st_size>1024,files
        host(r,0xffdf,.6);r.input(0xffdf,1);time.sleep(.4);r.release()
        changed=observe(r,'moved');assert all(changed['players'][i]['x']>saved['players'][i]['x']+15 for i in (0,1))
        state_operation(r,'load');restored=observe(r,'loaded')
        for i in (0,1):assert abs(restored['players'][i]['x']-saved['players'][i]['x'])<4
        r.screenshot(qa/'loaded-bodies.png')
        # Exercise new texture IDs and both native attacks on the new timeline.
        r.input(0x7fff,1);time.sleep(.5);r.release();host(r,0xdfff);time.sleep(.25)
        state_operation(r,'load');observe(r,'loaded-after-attacks');r.screenshot(qa/'loaded-after-attacks.png')
        r.fixture(2,1,0x80);deadline=time.monotonic()+10
        while r.players()[1][0x5c]&127:
            assert time.monotonic()<deadline,'P2 death did not begin';time.sleep(.05)
        observe(r,'partner-death');state_operation(r,'load')
        revived=observe(r,'loaded-before-partner-death')
        assert all(b['active']and b['hp']==32 for b in revived['players'])
        # Actual native pause state must restore its controlling player too.
        r.input(0xfff7,1);until(r,'overlay_status','save_menu_open',False,.5)
        deadline=time.monotonic()+10
        while r.read(0x801721c0,2)!=bytes((6,2)) or r.read(0x801754a4,1)[0]!=1:
            assert time.monotonic()<deadline,'P2 pause did not settle';time.sleep(.05)
        r.release();state_operation(r,'save');observe(r,'paused-save')
        r.input(0xfff7,1);deadline=time.monotonic()+10
        while r.read(0x801721c0,2)!=bytes((6,0)):
            assert time.monotonic()<deadline;time.sleep(.05)
        r.release();state_operation(r,'load');assert r.read(0x801721c0,2)==bytes((6,2));observe(r,'paused-load')
        r.input(0xfff7,1);deadline=time.monotonic()+10
        while r.read(0x801721c0,2)!=bytes((6,0)):
            assert time.monotonic()<deadline,'P2 cannot resume restored pause';time.sleep(.05)
        r.release();time.sleep(.3);r.wait_playable(10)
        state_operation(r,'save');cold_reference=observe(r,'cold-reference')
        # The real mapped rewind shortcut, navigation, accept and back.
        time.sleep(.8);host(r,0xffdf,.7);r.input(0xffdf,1);time.sleep(.5);r.release()
        moved=observe(r,'rewind-moved');assert moved['rewind']['snaps']>3,moved
        host(r,0xfffa);until(r,'rewind_status','open',True)
        for _ in range(8):host(r,0xff7f,.07)
        host(r,0xbfff);until(r,'rewind_status','open',False);time.sleep(.2)
        back=observe(r,'rewound')
        # The selected snapshot can be after P1 stopped but before P2 moved.
        # Require an actual earlier team position without assuming ring cadence.
        assert any(back['players'][i]['x']<moved['players'][i]['x']-4 for i in (0,1))
        assert all(b['active']and b['hp']for b in back['players'])
        r.screenshot(qa/'rewound-bodies.png')
        host(r,0xfffa);until(r,'rewind_status','open',True);host(r,0xdfff);until(r,'rewind_status','open',False)
        stop(r);r=launch(cold=True);r.wait_playable(60)
        cold=observe(r,'fresh-process-load')
        for i in (0,1):
            assert cold['players'][i]['character']==cold_reference['players'][i]['character']
            assert abs(cold['players'][i]['x']-cold_reference['players'][i]['x'])<4
            assert cold['players'][i]['active']and cold['players'][i]['hp']
        r.screenshot(qa/'fresh-process-bodies.png')
        r.input(0x7fff,1);time.sleep(.6);r.release();host(r,0xdfff);observe(r,'fresh-process-attacks')
        time.sleep(1.5);stop(r)
        print('PASS: native co-op mappings, repeated loads, pause owner, rewind, fresh-process state',flush=True)
    finally:stop()

if __name__=='__main__':main()
