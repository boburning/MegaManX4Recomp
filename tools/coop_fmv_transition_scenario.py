"""Native movie gateways with explicitly bypassed campaign prerequisites.

The offline fixture positions the dispatcher immediately before its authored
movie call. It never sets movie completion, dialogue completion, boss victory
or a next-scene result. Neutral input observes natural endings; the skip variant
uses native pad1 Start after MDEC and XA have both progressed.
"""
import json
import struct
import time


def counters(r):
    return {c['name']:c['count'] for c in r.request('mod_counters')['counters']}


def _fmv_transitions_once(s):
    r=s.r;results=[]
    for movie in s.fmv_ids:
        stage=11 if movie in (4,8) else 12 if movie in (5,9,10) else 9 if movie==3 else 0
        section=int(movie in (5,10))
        s.load(stage,section)
        r.fixture(5,3);r.release();time.sleep(.2)
        before=counters(r)
        start_key=f'mmx4.qa.movie{movie}.native-starts'
        return_key=f'mmx4.qa.movie{movie}.native-returns'
        fmv_before=r.request('fmv_state')
        if getattr(s,'fmv_turbo',False):r.request('turbo',enabled=1)
        r.fixture(8,movie)
        entry_deadline=time.monotonic()+90
        while time.monotonic()<entry_deadline:
            count=counters(r)
            if count.get(start_key,0)>before.get(start_key,0):break
            # These dispatchers admit their film without frontend dialogue.
            # Leave their pad neutral until the observer sees the call;
            # a confirm edge delivered across that boundary can skip it.
            if movie in (5,8,9,10):
                time.sleep(.15)
                continue
            # X's last story movie is admitted by selecting Final Weapon
            # after progress8. The original board uses cursor8 for that icon.
            mode=list(r.read(0x801721c0,4))
            if movie==4 and mode[:2]==[3,4] and mode[3]!=8:
                r.input(0xffbf,0);time.sleep(.08);r.release();time.sleep(.25)
                continue
            # Native story text can precede its movie. Release each confirm
            # pulse before the CD/MDEC admission, then remain neutral.
            r.input(0xbfff,0);time.sleep(.04);r.release();time.sleep(.15)
        else:raise AssertionError(('Authored gateway did not call the expected movie',movie,count))
        r.release()
        if getattr(s,'fmv_turbo',False):r.request('turbo',enabled=1)
        rows=[];started=time.monotonic();next_capture=3;skipped=False;saw_xa=False
        end=started+300
        while time.monotonic()<end:
            count=counters(r);fmv=r.request('fmv_state')
            row=dict(elapsed=time.monotonic()-started,mode=list(r.read(0x801721c0,4)),
                fmv=fmv,native_frame=struct.unpack('<I',r.read(0x801441c0,4))[0],
                native_total=struct.unpack('<I',r.read(0x801395e0,4))[0])
            rows.append(row);saw_xa|=bool(fmv['xa_stream_active'])
            if row['elapsed']>=next_capture:
                try:r.request('screenshot_file',path=str(s.output/f'movie-{movie}-{int(next_capture):03}.png'))
                except RuntimeError as error:
                    if 'display disabled' not in str(error):raise
                next_capture+=15
            decoded=fmv['mdec_decode_count']-fmv_before['mdec_decode_count']
            if s.fmv_skip and not skipped and decoded>30 and saw_xa and count.get(return_key,0)==before.get(return_key,0):
                r.input(0xfff7,0);time.sleep(.15);r.release();skipped=True
            if count.get(return_key,0)>before.get(return_key,0):break
            time.sleep(.2)
        else:raise AssertionError(('Movie failed to return naturally',movie,rows[-1]))
        assert count.get(start_key,0)==before.get(start_key,0)+1,('Wrong/duplicate native movie',movie,count)
        assert decoded>30 and saw_xa,('Missing native video/audio progression',movie,decoded,saw_xa)
        if s.fmv_skip:
            assert skipped,('Skip input not exercised',movie)
            terminal=count.get(f'mmx4.qa.movie{movie}.last-frame',0)-before.get(f'mmx4.qa.movie{movie}.last-frame',0)
            native_total=struct.unpack('<I',r.read(0x801395e0,4))[0]
            assert terminal<native_total,('Skip did not shorten native playback',movie,terminal,native_total)
        else:
            terminal_key=f'mmx4.qa.movie{movie}.last-frame'
            terminal=count.get(terminal_key,0)-before.get(terminal_key,0)
            native_total=struct.unpack('<I',r.read(0x801395e0,4))[0]
            assert terminal>=native_total,('Movie returned before its authored end',movie,terminal,native_total)
        r.release()
        if getattr(s,'fmv_turbo',False):r.request('turbo',enabled=0)
        after=s.sample(f'movie-{movie}-native-return')
        try:r.request('screenshot_file',path=str(s.output/f'movie-{movie}-native-return.png'))
        except RuntimeError as error:
            if 'display disabled' not in str(error):raise
        assert after['mode'][0] in (3,4,5,6,9,10,11,12),after
        assert not after['plugin_failed'],after
        if movie in (5,10):
            assert after['mode'][0]==11 and after['mode'][1]>=1,('Ending did not continue',after)
        else:
            continued_deadline=time.monotonic()+90;save_choice=False
            while time.monotonic()<continued_deadline:
                row=s.sample(f'movie-{movie}-continuation')
                if row['mode'][:2]==[6,0] and not row['blocked'] and all(
                    p['active'] and p['visible'] and p['hp'] and p['state']==1 and
                    p['action']>=2 for p in row['players']):break
                if row['mode'][:2]==[9,1] and not save_choice:
                    # Colonel's interlude returns through the original save
                    # prompt. Choose its second (continue without saving)
                    # option with native Down before confirming.
                    r.input(0xffbf,0);time.sleep(.12);r.release();save_choice=True
                elif row['mode'][0] in (3,9,12) or row['blocked']:
                    r.input(0xbfff,0);time.sleep(.08);r.release()
                time.sleep(.1)
            else:raise AssertionError(('Native movie sequel did not release',movie,row))
            # A dispatcher can publish gameplay before the first visible GPU
            # submission after a frontend interlude.
            time.sleep(.4)
            after=s.sample(f'movie-{movie}-continued-gameplay',True)
            assert after['mode'][:2]==[6,0] and all(p['active'] and p['visible'] and p['hp']
                for p in after['players']),('Movie did not continue to both players',movie,after)
        (s.output/f'movie-{movie}-timeline.json').write_text(json.dumps(rows,indent=2))
        result=dict(movie=movie,skipped=skipped,turbo=getattr(s,'fmv_turbo',False),elapsed=time.monotonic()-started,
            decoded=decoded,xa_seen=saw_xa,native_return=True,continued_mode=after['mode'],
            stage=after['stage'],section=after['section'])
        results.append(result)
        print(json.dumps(result),flush=True)
    return dict(movies=results,campaign_prerequisites_bypassed=True)


def state_operation(r, operation):
    before=r.request('savestate_status')['generation']
    r.request('savestate',op=operation,slot=10)
    deadline=time.monotonic()+20
    while time.monotonic()<deadline:
        state=r.request('savestate_status')
        if state['generation']>before and not state['pending']:
            assert state['last_ok'] and state['last_op']==operation,state
            return
        time.sleep(.05)
    raise TimeoutError((operation,state))


def fmv_transitions(s):
    # A current-build execution snapshot restores campaign prerequisites after
    # each native ending. It never supplies a movie-completion result.
    r=s.r;output=s.output;ids=s.fmv_ids;skip=s.fmv_skip;results=[]
    r.release();time.sleep(.2);state_operation(r,'save')
    try:
        variants=(False,True) if getattr(s,'fmv_both',False) else (skip,)
        for skipping in variants:
            for movie in ids:
                state_operation(r,'load');r.release();time.sleep(.2);r.wait_playable(30)
                s.output=output/f"movie-{movie}-{'skip' if skipping else 'natural'}"
                s.output.mkdir()
                s.fmv_ids=[movie];s.fmv_skip=skipping
                result=_fmv_transitions_once(s)
                results.extend(result['movies'])
                (output/'movies-report.json').write_text(json.dumps(results,indent=2))
        state_operation(r,'load');r.wait_playable(30)
    finally:
        if getattr(s,'fmv_turbo',False):r.request('turbo',enabled=0)
        s.output=output;s.fmv_ids=ids;s.fmv_skip=skip
    return dict(movies=results,campaign_prerequisites_bypassed=True)


def opening_transition(r, output, turbo=False):
    # Cold boot into the authored attract movie with neutral native controls.
    r.release();before=r.request('fmv_state');rows=[];saw_xa=False
    deadline=time.monotonic()+300;started=None;next_capture=3
    try:
        while time.monotonic()<deadline:
            count=counters(r);fmv=r.request('fmv_state')
            if count.get('mmx4.qa.movie1.native-starts',0):
                if started is None:
                    started=time.monotonic()
                    if turbo:r.request('turbo',enabled=1)
                elapsed=time.monotonic()-started
                rows.append(dict(elapsed=elapsed,fmv=fmv))
                saw_xa|=bool(fmv['xa_stream_active'])
                if elapsed>=next_capture:
                    try:r.request('screenshot_file',path=str(output/f'opening-natural-{int(next_capture):03}.png'))
                    except RuntimeError as error:
                        if 'display disabled' not in str(error):raise
                    next_capture+=15
                if count.get('mmx4.qa.movie1.native-returns',0):break
            time.sleep(.15)
        else:raise AssertionError('Neutral cold boot opening did not complete')
        total=struct.unpack('<I',r.read(0x801395e0,4))[0]
        assert count['mmx4.qa.movie1.last-frame']>=total and saw_xa
        assert fmv['mdec_decode_count']-before['mdec_decode_count']>30
        result=dict(movie=1,skipped=False,turbo=turbo,native_return=True,xa_seen=saw_xa,
                    terminal=count['mmx4.qa.movie1.last-frame'],total=total)
        (output/'opening-natural.json').write_text(json.dumps(dict(result=result,rows=rows),indent=2))
        print(json.dumps(result),flush=True)
        return result
    finally:
        if turbo:r.request('turbo',enabled=0)
