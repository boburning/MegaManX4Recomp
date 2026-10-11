"""Generate real input-only diagnostic sessions in isolated, hidden game copies.

Requires a development executable with TCP tools. Never touches personal cards,
uses desktop input, or injects native stage/health/teleport fixtures.
"""
import argparse,json,os,shutil,socket,struct,subprocess,time
from pathlib import Path
from coop_debug import Runtime
from test_mmx4_coop_netplay import change_config

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--game',type=Path,required=True)
    p.add_argument('--bios',type=Path,required=True)
    p.add_argument('--disc',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--campaign',type=int,choices=(0,1),default=0)
    p.add_argument('--launcher',action='store_true')
    args=p.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    original=args.exe.resolve(strict=True);exe=out/original.name
    shutil.copy2(original,exe)
    for name in ('assets','mods'):
        shutil.copytree(original.parent/name,out/name)
    for dll in original.parent.glob('*.dll'):shutil.copy2(dll,out/dll.name)
    (out/'mods/state.toml').write_text('format_version = 2\n\n[[feature]]\npackage_id = "mmx4.coop"\nid = "coop"\nenabled = true\n')
    config=args.game.resolve().read_text()
    values=[('game','exe',str(Path(__file__).resolve().parents[1]/'mmx4/SLUS_005.61')),
            ('runtime','memcard_dir',str(out/'private-cards')),('runtime','overlay_autocompile_cmd','')]
    for section,key,value in values:config=change_config(config,section,key,value)
    (out/'game.toml').write_text(config)
    with socket.socket()as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
    env={k:v for k,v in os.environ.items()if not k.startswith(('PSX_','RNET_','LNG_','MMX4_DIAGNOSTIC_'))
         and k not in ('APPIMAGE','APPDIR','RECOMP_APPIMAGE_PATH')}
    argv=[str(exe),'--launcher'if args.launcher else '--no-launcher','--hidden-window','--renderer','opengl',
          '--game',str(out/'game.toml'),'--bios',str(args.bios.resolve()),'--disc',str(args.disc.resolve()),
          '--memcard-dir',str(out/'private-cards'),'--debug-port',str(port)]
    if args.launcher:env.update(LNG_TEST_HIDDEN='1',LNG_TCP_PORT='0',LNG_TCP_PORT_FILE=str(out/'launcher-port.txt'))
    flags=subprocess.CREATE_NO_WINDOW if os.name=='nt'else 0
    with (out/'runtime.log').open('wb')as log:
        proc=subprocess.Popen(argv,cwd=out,env=env,stdout=log,stderr=log,creationflags=flags)
    r=Runtime(port)
    try:
        deadline=time.monotonic()+60
        if args.launcher:
            while not (out/'launcher-port.txt').exists():
                assert proc.poll()is None and time.monotonic()<deadline,'Launcher unavailable';time.sleep(.1)
            with socket.create_connection(('127.0.0.1',int((out/'launcher-port.txt').read_text())),timeout=20)as s:
                f=s.makefile('rb')
                def command(c):
                    s.sendall((c+'\n').encode());v=json.loads(f.readline());assert v.get('ok'),(c,v);return v
                state=command('state');assert state['hidden']and state['window_hidden'],state
                command('size:1120x860');command('wait:30');state=command('state')
                command('shot:'+str(out/'launcher.png'))
                command(f"click:{state['width']-135},{state['height']-61}")
                f.close()
        while True:
            try:r.read(0x801721c0,4);break
            except OSError:
                assert proc.poll()is None and time.monotonic()<deadline,'Runtime unavailable';time.sleep(.1)
        r.boot(campaign=args.campaign)
        def pulse(buttons,seat,duration=.3):r.input(buttons,seat);time.sleep(duration);r.release();time.sleep(.15)
        pulse(0xffdf,0);pulse(0xffdf,1);pulse(0x7fff,1,.6);pulse(0xdfff,1);pulse(0x7fff,0,.6)
        for owner in (0,1):
            pulse(0xfff7,owner)
            until=time.monotonic()+10
            while r.read(0x801721c0,2)!=bytes((6,2)) or r.read(0x801754a4,1)[0]!=1:
                assert time.monotonic()<until,'Pause did not settle';time.sleep(.06)
            r.input(0xfff7,owner)
            until=time.monotonic()+10
            while r.read(0x801721c0,2)!=bytes((6,0)):
                assert time.monotonic()<until,'Resume did not settle';time.sleep(.06)
            r.release();time.sleep(.3)
        # The native quit handler exits after closing the listener and can
        # end the connection before the pending acknowledgement is delivered.
        try:r.request('quit')
        except (OSError,RuntimeError):pass
        proc.wait(timeout=15)
        assert proc.returncode==0,proc.returncode
        sessions=list((out/'saves/diagnostics').glob('session-*'));assert len(sessions)==1,sessions
        session=sessions[0];lines=(session/'inputs.csv').read_text().splitlines()
        assert str(session)in (out/'diagnostics-location.txt').read_text()
        assert not json.loads((session/'session.json').read_text())['netplay']
        rows=[line for line in lines if line.startswith('N ')]
        events=[line for line in lines if line.startswith('C ')]
        assert len(rows)>200 and events,(len(rows),len(events))
        assert (session/'mods-state.toml').is_file()
        for slot in (1,2):assert (session/f'card{slot}.mcd').stat().st_size==131072
        assert any(int(line[2:].split(',')[3])!=65535 for line in rows),'P2 input was not captured'
        report=dict(passed=True,campaign=args.campaign,launcher=args.launcher,session=str(session),
                    native_inputs=len(rows),coop_reads=len(events),exit_code=proc.returncode)
        (out/'report.json').write_text(json.dumps(report,indent=2));print(json.dumps(report),flush=True)
    finally:
        if proc.poll()is None:proc.terminate();proc.wait(timeout=15)

if __name__=='__main__':main()
