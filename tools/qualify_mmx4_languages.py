"""Muted, isolated native language smoke test using owner-provided discs.

Retains screenshots and CD/SPU observations for review. Asset qualification
is a separate C++ test; this does not certify every story scene or audio cue.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import socket
import struct
import subprocess
import time
import zipfile
from coop_debug import Runtime
from test_mmx4_coop_netplay import change_config
from build_translation_mod import build


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('exe','game','disc','output'):parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--jp-disc',type=Path)
    parser.add_argument('--japanese',nargs='*',choices=('fmvs','voices','title-screen'),default=[])
    parser.add_argument('--catalog',type=Path,help='Package an edited catalog, exercising the community authoring path')
    parser.add_argument('--campaign',type=int,choices=(0,1),default=0)
    parser.add_argument('--coop',action='store_true')
    args=parser.parse_args()
    qa=args.output.resolve()
    if os.name=='nt' and qa.drive.lower()=='c:':parser.error('Qualification output must be on a non-C: drive')
    if args.japanese and not args.jp_disc:parser.error('--japanese requires --jp-disc')
    qa.mkdir(parents=True,exist_ok=False)
    source=args.exe.resolve(strict=True);exe=qa/source.name
    shutil.copy2(source,exe)
    for name in ('assets','mods','bios'):
        if (source.parent/name).exists():shutil.copytree(source.parent/name,qa/name)
    for dll in source.parent.glob('*.dll'):shutil.copy2(dll,qa/dll.name)
    state='format_version = 2\n'
    if args.japanese:
        state+='[[feature]]\npackage_id = "mmx4.localization"\nid = "japanese-media"\nenabled = true\n[feature.values]\n'
        state+=''.join(f'{key} = {str(key in args.japanese).lower()}\n' for key in ('fmvs','voices','title-screen'))
        state+='[feature.resources]\njapanese-disc = '+json.dumps(args.jp_disc.resolve().as_posix())+'\n'
    state+='[[feature]]\npackage_id = "mmx4.localization"\nid = "translation"\nenabled = true\n'
    if args.coop:
        state+='[[feature]]\npackage_id = "mmx4.coop"\nid = "coop"\nenabled = true\n[feature.values]\ncameras = "unified"\nlocal_cameras = "unified"\n'
    (qa/'mods/state.toml').write_text(state,encoding='utf-8')
    if args.catalog:
        archive=qa/'community-translation.psxmod';build(args.catalog,archive)
        with zipfile.ZipFile(archive) as package:
            package.extractall(qa/'mods/installed/mmx4.localization/1.0.0')
    (qa/'settings.toml').write_text('[video]\nfullscreen = false\ninternal_resolution = "native"\nwindow_width = 640\n[audio]\nvolume = 0\n')
    cfg=args.game.resolve().read_text(encoding='utf-8')
    game_exe=args.game.resolve().parent/'mmx4/SLUS_005.61'
    for section,key,value in [('game','exe',game_exe.as_posix()),('video','internal_resolution','native'),
            ('runtime','memcard_dir',(qa/'cards').as_posix()),('runtime','overlay_autocompile_cmd','')]:
        cfg=change_config(cfg,section,key,value)
    (qa/'game.toml').write_text(cfg,encoding='utf-8')
    env=os.environ.copy()
    for key,relative in dict(TEMP='temp',TMP='temp',LOCALAPPDATA='profile/Local',APPDATA='profile/Roaming',
            XDG_CACHE_HOME='cache',XDG_DATA_HOME='data',PSX_RESIDENT_CACHE='resident',PSX_OVERLAY_CACHE_DIR='overlay-cache',
            PSX_OVERLAY_CAPTURE_ROOT='overlay-capture',PYTHONPYCACHEPREFIX='pycache',__GL_SHADER_DISK_CACHE_PATH='shader-cache',
            MESA_SHADER_CACHE_DIR='shader-cache',CUDA_CACHE_PATH='cuda-cache').items():
        value=qa/relative;value.mkdir(parents=True,exist_ok=True);env[key]=str(value)
    env.update(SDL_AUDIODRIVER='dummy',PSX_OVERLAY_AUTOCOMPILE_OFF='1',PSX_OVERLAY_CAPTURES=str(qa/'overlay_captures.json'))
    with socket.socket() as sock:sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
    with (qa/'runtime.log').open('wb') as log:
        process=subprocess.Popen([str(exe),'--hidden-window','--no-launcher','--renderer','opengl','--game',str(qa/'game.toml'),
            '--disc',str(args.disc.resolve()),'--memcard-dir',str(qa/'cards'),'--debug-port',str(port)],cwd=qa,env=env,
            stdout=log,stderr=log,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    (qa/'owned.json').write_text(json.dumps(dict(pid=process.pid,port=port)))
    r=Runtime(port);report=dict(campaign=args.campaign,coop=args.coop,japanese=args.japanese,status='running')
    try:
        deadline=time.monotonic()+60
        while True:
            try:r.request('gpu_state');break
            except OSError:
                assert process.poll() is None and time.monotonic()<deadline,'Runtime unavailable'
                time.sleep(.2)
        time.sleep(20 if 'fmvs' in args.japanese else 3)
        report['opening_fmv']=r.request('fmv_state');report['opening_cd']=r.request('cdrom_state')
        try:r.request('screenshot_file',path=str(qa/'opening.png'))
        except RuntimeError:pass # scanout can still be disabled during boot
        deadline=time.monotonic()+100
        while r.read(0x80173c70,2)!=bytes([6,1]):
            assert time.monotonic()<deadline,'Title not reached'
            r.input(0xfff7);time.sleep(.12);r.release();time.sleep(.6)
        time.sleep(2);r.request('screenshot_file',path=str(qa/'title.png'))
        deadline=time.monotonic()+140;captures=0;iteration=0
        while True:
            play=r.read(0x801721c0,0x20)
            if play[0]==6:break
            assert time.monotonic()<deadline,'Gameplay not reached'
            if (play[0x1c] or play[:2]==bytes([3,9])) and captures<8:
                time.sleep(4.5)
                r.request('screenshot_file',path=str(qa/f'dialogue-{captures}.png'));captures+=1
                report.setdefault('dialogue_cd',[]).append(r.request('cdrom_state'))
                print('Captured native dialogue',captures,flush=True)
            buttons=0xfff7 if iteration%2==0 else 0xbfff
            r.input(buttons & (0xffdf if args.campaign else 0xffff));time.sleep(.2);r.release();time.sleep(.5);iteration+=1
        time.sleep(2);r.request('screenshot_file',path=str(qa/'intro.png'))
        if args.coop:
            bodies=r.wait_playable(30,advance_dialogue=True)
            report['characters']=[p[2] for p in bodies]
            assert report['characters']==[args.campaign,args.campaign^1],'Co-op campaign mismatch'
            r.screenshot(qa/'coop.png')
        r.input(0xbfff);time.sleep(.25);r.release();time.sleep(.5)
        report['spu']=r.request('spu_status');report['counters']=r.request('mod_counters')
        if 'voices' in args.japanese:
            spu=b''.join(bytes.fromhex(r.request('spu_ram',addr=i,len=4096)['hex']) for i in range(0,512*1024,4096))
            (qa/'spu.bin').write_bytes(spu)
            # Match Japanese-specific sample blocks in the actual native SPU RAM.
            import sys
            sys.path.insert(0,str(args.game.resolve().parent/'psxrecomp-v4/tools'))
            from aot_overlay_pipeline import Disc
            def voice_body(arc):
                at=2048
                for n in range(struct.unpack_from('<I',arc)[0]):
                    kind,size=struct.unpack_from('<II',arc,8+n*8)
                    if kind==0x20000:return arc[at:at+size]
                    at=(at+size+2047)&~2047
                raise RuntimeError('Native player VAB body missing')
            name=f'PL{args.campaign:02}'
            jp=voice_body(Disc(args.jp_disc.resolve()).read(f'ARC/{name}.ARC'))
            us=voice_body(Disc(args.disc.resolve()).read(f'ARC/{name}_U.ARC'))
            report['japanese_spu_blocks']=[dict(donor_offset=i,spu_address=spu.find(jp[i:i+512]))
                for i in range(0,len(jp)-512,512) if jp[i:i+512]!=us[i:i+512] and any(jp[i:i+512]) and spu.find(jp[i:i+512])>=0]
            assert report['japanese_spu_blocks'],'Japanese-specific voice samples did not reach SPU'
        assert captures,'No native dialogue captured'
        assert any(c['name']=='mmx4.language.tables_published' for c in report['counters']['counters'])
        report['status']='passed';print('PASS: native language smoke test; screenshots need visual review',flush=True)
    except Exception as error:
        report.update(status='failed',error=str(error));raise
    finally:
        (qa/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        if process.poll() is None:
            try:r.request('quit')
            except (OSError,RuntimeError):pass
            try:process.wait(timeout=15)
            except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=15)

if __name__=='__main__':main()
