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
    parser.add_argument('--fmv-mod',type=Path,help='Imported movie-only .psxmod')
    parser.add_argument('--fmv-pack',type=Path,help='Matching verified .x4fmv resource')
    parser.add_argument('--fmv-language',choices=('en','es'),default='en')
    parser.add_argument('--fmv-watch-story',action='store_true',help='Capture and allow the first campaign movie to finish naturally before dialogue input')
    parser.add_argument('--japanese',nargs='*',choices=('fmvs','voices','title-screen'),default=[])
    parser.add_argument('--catalog',type=Path,help='Package an edited catalog, exercising the community authoring path')
    parser.add_argument('--language',choices=('es','pt-br'),default='es')
    parser.add_argument('--resolution',choices=('native','1080p'),default='native')
    parser.add_argument('--window-width',type=int,default=640)
    parser.add_argument('--original-loader',action='store_true',help='Qualify loading artwork on the original CD path')
    parser.add_argument('--campaign',type=int,choices=(0,1),default=0)
    parser.add_argument('--coop',action='store_true')
    args=parser.parse_args()
    qa=args.output.resolve()
    if os.name=='nt' and qa.drive.lower()=='c:':parser.error('Qualification output must be on a non-C: drive')
    if args.japanese and not args.jp_disc:parser.error('--japanese requires --jp-disc')
    if args.fmv_mod and not args.fmv_pack:parser.error('--fmv-mod requires --fmv-pack')
    qa.mkdir(parents=True,exist_ok=False)
    source=args.exe.resolve(strict=True);exe=qa/source.name
    shutil.copy2(source,exe)
    for name in ('assets','mods','bios'):
        if (source.parent/name).exists():shutil.copytree(source.parent/name,qa/name)
    for dll in source.parent.glob('*.dll'):shutil.copy2(dll,qa/dll.name)
    state='format_version = 2\n'
    if args.original_loader:
        state+='[[feature]]\npackage_id = "mmx4.enhancement.resident-loading"\nid = "resident-loading"\nenabled = false\n'
    if set(args.japanese)&{'voices','title-screen'}:
        state+='[[feature]]\npackage_id = "mmx4.localization"\nid = "japanese-media"\nenabled = true\n[feature.values]\n'
        state+=''.join(f'{key} = {str(key in args.japanese).lower()}\n' for key in ('voices','title-screen'))
        state+='[feature.resources]\njapanese-disc = '+json.dumps(args.jp_disc.resolve().as_posix())+'\n'
    state+='[[feature]]\npackage_id = "mmx4.localization"\nid = "translation"\nenabled = true\n'
    state+='[feature.values]\nlanguage = '+json.dumps(args.language)+'\n'
    if args.coop:
        state+='[[feature]]\npackage_id = "mmx4.coop"\nid = "coop"\nenabled = true\n[feature.values]\ncameras = "unified"\nlocal_cameras = "unified"\n'
    if args.fmv_pack or 'fmvs' in args.japanese:
        selected=args.fmv_language if args.fmv_pack else 'none'
        resource={'en':'english-movies','es':'spanish-movies','none':'japanese-disc'}[selected]
        media=args.fmv_pack if args.fmv_pack else args.jp_disc
        state+='[[feature]]\npackage_id = "mmx4.japanese-fmvs"\nid = "japanese-fmvs"\nenabled = true\n'
        state+='[feature.values]\nsubtitles = '+json.dumps(selected)+'\n[feature.resources]\n'
        state+=resource+' = '+json.dumps(media.resolve().as_posix())+'\n'
    (qa/'mods/state.toml').write_text(state,encoding='utf-8')
    if args.fmv_mod:
        with zipfile.ZipFile(args.fmv_mod) as package:
            expected={'manifest.toml','provenance.json','CREDITS.md','upstream/acediez-Undub-v1.13-Readme.txt',
                      'upstream/El-Retro-Dan-Final-LEEME.txt','provenance/en.json','provenance/es.json'}
            if not set(package.namelist())<=expected:raise ValueError('Unexpected movie mod payload')
            package.extractall(qa/'mods/installed/mmx4.japanese-fmvs/1.0.0')
    if args.catalog:
        archive=qa/'community-translation.psxmod';build(args.catalog,archive)
        with zipfile.ZipFile(archive) as package:
            package.extractall(qa/'mods/installed/mmx4.localization/1.0.0')
    (qa/'settings.toml').write_text(f'[video]\nfullscreen = false\ninternal_resolution = "{args.resolution}"\nwindow_width = {args.window_width}\n[audio]\nvolume = 0\n')
    cfg=args.game.resolve().read_text(encoding='utf-8')
    game_exe=args.game.resolve().parent/'mmx4/SLUS_005.61'
    for section,key,value in [('game','exe',game_exe.as_posix()),('video','internal_resolution',args.resolution),
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
    r=Runtime(port);report=dict(campaign=args.campaign,coop=args.coop,japanese=args.japanese,language=args.language,
        fmv_language=args.fmv_language if args.fmv_pack else None,
        original_loader=args.original_loader,resolution=args.resolution,status='running')
    try:
        deadline=time.monotonic()+60
        while True:
            try:r.request('gpu_state');break
            except OSError:
                assert process.poll() is None and time.monotonic()<deadline,'Runtime unavailable'
                time.sleep(.2)
        time.sleep(20 if 'fmvs' in args.japanese or args.fmv_pack else 3)
        report['opening_fmv']=r.request('fmv_state');report['opening_cd']=r.request('cdrom_state')
        if args.fmv_pack or 'fmvs' in args.japanese:
            report['opening_movie_table']=struct.unpack('<III',r.read(0x800f149c,12))
            assert report['opening_movie_table'][0]>=251793,'Selected movies did not replace the native opening table'
        try:r.request('screenshot_file',path=str(qa/'opening.png'))
        except RuntimeError:pass # scanout can still be disabled during boot
        deadline=time.monotonic()+100
        while r.read(0x80173c70,2)!=bytes([6,1]):
            assert time.monotonic()<deadline,'Title not reached'
            r.input(0xfff7);time.sleep(.12);r.release();time.sleep(.6)
        # Front state 6/1 starts the logo's entrance animation, not its end.
        # Let that real animation finish before accepting the idle title.
        time.sleep(7);r.screenshot(qa/'title.png')
        report['resident_at_title']=r.request('resident_status')
        report['title_perf']=r.request('frame_perf')
        # Capture the actual menu-to-character-select transition, including
        # the loading-logo animation. Inputs and native state stay authoritative.
        started=time.monotonic();r.input(0xfff7);time.sleep(.12);r.release()
        report['start_transition']=[]
        for i in range(40):
            now=time.monotonic();play=r.read(0x801721c0,2);front=r.read(0x80173c70,2)
            report['start_transition'].append(dict(seconds=now-started,play=play.hex(),front=front.hex()))
            try:r.screenshot(qa/f'start-{i:02}.png')
            except RuntimeError as error:
                if 'display disabled' not in str(error):raise
            time.sleep(.1)
        report['resident_after_start']=r.request('resident_status')
        report['resident_events_after_start']=r.request('resident_events')
        report['start_perf']=r.request('frame_perf')
        if not args.original_loader:
            packs=report['resident_at_title']['packs']
            assert packs and not packs[0]['error'],'Effective language assets failed resident preparation'
            assert all(e['served'] for e in report['resident_events_after_start']['entries']), 'A blocking archive fell back to CD during Game Start'
        # Confirm the character through native input, retaining the subsequent
        # loading logo as well as the first movie/story transition.
        if args.campaign:r.input(0xffdf);time.sleep(.18);r.release();time.sleep(.3)
        r.input(0xbfff);time.sleep(.12);r.release()
        for i in range(45):
            try:r.screenshot(qa/f'loading-{i:02}.png')
            except RuntimeError as error:
                if '24bpp' in str(error):r.request('screenshot_file',path=str(qa/f'loading-{i:02}.png'))
                elif 'display disabled' not in str(error):raise
            time.sleep(.03)
        if args.fmv_pack and args.fmv_watch_story:
            table=0x800f14d8 if args.campaign else 0x800f14a8
            lba,length,header=struct.unpack('<III',r.read(table,12))
            report['story_movie']=dict(lba=lba,bytes=length,first=header,before=r.request('fmv_state'),samples=[])
            started=time.monotonic()
            for sample in range(8):
                time.sleep(3)
                report['story_movie']['samples'].append(dict(elapsed=time.monotonic()-started,
                    fmv=r.request('fmv_state'),cd=r.request('cdrom_state')))
                r.request('screenshot_file',path=str(qa/f'story-fmv-{sample:02}.png'))
            # Two-speed native CD timeline. Waiting observes natural movie
            # completion; it never changes the movie ID or success flags.
            remaining=length/2336/150+4-(time.monotonic()-started)
            while remaining>0:
                time.sleep(min(remaining,1));remaining=length/2336/150+4-(time.monotonic()-started)
            report['story_movie']['after']=r.request('fmv_state')
            r.request('screenshot_file',path=str(qa/'story-fmv-completed.png'))
        deadline=time.monotonic()+140;captures=0;iteration=0
        while True:
            play=r.read(0x801721c0,0x20)
            if play[0]==6:break
            assert time.monotonic()<deadline,'Gameplay not reached'
            if (play[0x1c] or play[:2]==bytes([3,9])) and captures<8:
                time.sleep(4.5)
                r.screenshot(qa/f'dialogue-{captures}.png');captures+=1
                report.setdefault('dialogue_cd',[]).append(r.request('cdrom_state'))
                print('Captured native dialogue',captures,flush=True)
            buttons=0xfff7 if iteration%2==0 else 0xbfff
            r.input(buttons & (0xffdf if args.campaign else 0xffff));time.sleep(.2);r.release();time.sleep(.5);iteration+=1
        time.sleep(2);r.screenshot(qa/'intro.png')
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
