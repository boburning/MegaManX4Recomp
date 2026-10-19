"""Qualify the development scaling fixture and real native state restoration.

This prepares matched-fight comparisons. It does not grade slash behavior.
"""
import json
import time


def balance_saved_control(s):
    r=s.r;results={}
    r.fixture(5,3);s.load(2,1,3);r.fixture(5,3)
    deadline=time.monotonic()+60
    while time.monotonic()<deadline:
        row=s.sample('saved-balance-admission')
        boss=next((e for e in s.enemies() if e['type']==57 and e['hp']==48),None)
        if (boss and boss['state'][0]==1 and boss['state'][1]>=4 and
                not row['blocked'] and not row['warning'] and not row['freeze'] and
                all(p['active'] and p['visible'] and p['hp'] and p['state']==1 and
                    p['action']==2 and not p['script'] and not p['door'] for p in row['players'])):
            break
        for seat in (0,1):
            r.input(0xbfff if row['blocked'] or row['warning'] else
                    (0xffff if boss and boss['state'][1]>=4 else 0xffdf),seat)
        time.sleep(.04);r.release()
    else:raise TimeoutError('Original Frost fight did not reach its native active state')
    r.release();results['before']=s.sample('saved-balance-before',True)
    results['boss_before']=boss
    def state_operation(op):
        previous=r.request('savestate_status')['generation']
        ack=r.request('savestate',op=op,slot=0)
        deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            status=r.request('savestate_status')
            if status['generation']!=previous and not status['pending']:
                assert status['last_ok'] and status['last_op']==op and status['last_slot']==0,status
                return dict(ack=ack,completed=status)
            time.sleep(.02)
        raise TimeoutError('Native savestate operation did not complete: '+op)
    results['save']=state_operation('save')
    r.fixture(6,253,0)
    results['reference_off']=s.sample('saved-balance-reference-off',True)
    counts={c['name']:c['count']for c in r.request('mod_counters')['counters']}
    assert counts.get('mmx4.coop.development-scaling-off',0)>0,counts
    # Existing snapshots strictly validate the current mode. Restore that
    # mode first; do not loosen validation to admit a cross-mode image.
    r.fixture(6,253,1)
    results['load']=state_operation('load')
    results['restored']=s.sample('saved-balance-restored',True)
    counts={c['name']:c['count']for c in r.request('mod_counters')['counters']}
    results['counters']=counts
    assert counts.get('mmx4.coop.development-scaling-on',0)>0,counts
    assert (results['restored']['stage'],results['restored']['section'])==(2,1)
    assert all(p['hp'] and p['visible'] for p in results['restored']['players']),results['restored']
    current=next(e for e in s.enemies() if e['type']==57 and e['slot']==boss['slot'])
    assert current['hp']==48,current
    results['boss_restored']=current
    (s.output/'saved-balance-control.json').write_text(json.dumps(results,indent=2))
    return dict(native_save_load=True,development_mode_fixture=True,
                slash_acceptance=False,boss_health=current['hp'])


def qualify_saved_inputs(result):
    """Qualify the recorded control, independently of damage or hit totals."""
    wanted=[buttons for frames,buttons in result['steps'] for _ in range(frames)]
    zero=result['zero_seat'];first,second=result['trials']
    for trial in (first,second):
        rows=trial['inputs'];assert len(rows)>=len(wanted),len(rows)
        assert [row[2+zero] for row in rows[:len(wanted)]]==wanted
        assert all(row[2+(zero^1)]==0xffff for row in rows[:len(wanted)])
    # Includes restored guest clock, controls, both heroes' observations and
    # the recorder's full PLAY/player-state hash throughout neutral setup.
    assert len(wanted)>=60 and all(x[1:]==y[1:] for x,y in
        zip(first['inputs'][:60],second['inputs'][:60]))
    return dict(native_input_frames=len(wanted),matched_setup_rows=60,
                saved_native_trials=2,slash_acceptance=False)


def balance_saved_slashes(s):
    """Record two native trials from one saved fight; never infer a PASS from HP."""
    balance_saved_control(s)
    r=s.r;zero=s.campaign^1;xseat=s.campaign
    boss=next(e for e in s.enemies() if e['type']==57 and e['hp']==48)
    r.fixture(7,zero,0,getattr(s,'saved_grant_skills',1))
    x_distance=getattr(s,'saved_x_distance',100)
    r.teleport(xseat,int(boss['x'])-x_distance,443)
    r.teleport(zero,int(boss['x'])-getattr(s,'saved_zero_distance',36),442)
    r.input(getattr(s,'saved_face_input',0xffdf),zero);time.sleep(.035);r.release()
    def operation(op):
        previous=r.request('savestate_status')['generation']
        ack=r.request('savestate',op=op,slot=0);deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            status=r.request('savestate_status')
            if status['generation']!=previous and not status['pending']:
                assert status['last_ok'] and status['last_op']==op,status
                return dict(ack=ack,completed=status)
            time.sleep(.02)
        raise TimeoutError('Saved slash operation: '+op)
    actor=0x8013bed0+boss['slot']*0x9c
    result=dict(save=operation('save'),zero_seat=zero,trials=[],x_distance=x_distance)
    steps=getattr(s,'saved_steps',None) or (
        [(60,0xffff)]+[(4,0xbfff),(3,0xffff),(4,0x7fff),(27,0xffff)]*6+[(60,0xffff)])
    result['steps']=steps
    sessions=s.output/'saves/diagnostics'
    for mode in (0,1):
        r.fixture(6,253,1)
        r.input(0xffff,xseat)
        r.request('input_route_clear')
        for frames,buttons in steps:r.request('input_route_append',frames=frames,buttons=buttons)
        armed=r.request('input_route_start',port=zero+1,after_load=1)
        assert armed['waiting_for_load'],armed
        r.request('fntrace_clear');r.request('wtrace_clear')
        r.request('wtrace_range',lo=hex(actor+0x58),hi=hex(actor+0x9c))
        for target in(0x8002de30,0x80097eec):r.request('fntrace_arm',target=hex(target))
        previous=set(sessions.glob('session-*'))
        trial=dict(mode=mode,armed=armed,load=operation('load'),rows=[])
        result['trials'].append(trial)
        status=r.request('input_route_status');trial['after_load_route']=status
        assert not status['waiting_for_load'] and status['index']==0 and status['remaining']>15,status
        r.fixture(6,253,mode)
        trial['counters_before']={c['name']:c['count'] for c in r.request('mod_counters')['counters']}
        trial['mode_applied_route']=r.request('input_route_status')
        assert trial['mode_applied_route']['index']==0,trial['mode_applied_route']
        deadline=time.monotonic()+45;last_capture=-1000;image_index=0
        while time.monotonic()<deadline:
            row=s.sample('saved-slash-'+str(mode));row['boss']=r.read(actor,0x9c).hex()
            trial['rows'].append(row)
            if row['frame']-last_capture>=16:
                r.screenshot(s.output/f'saved-slash-{mode}-{image_index:03d}.png')
                image_index+=1;last_capture=row['frame']
            status=r.request('input_route_status')
            if not status['active']:break
            assert all(p['hp'] and p['state']==1 for p in row['players']),row
            time.sleep(.01)
        else:raise TimeoutError('Saved slash route did not finish')
        r.release();time.sleep(.2)
        trial['counters_after']={c['name']:c['count'] for c in r.request('mod_counters')['counters']}
        trial['final_hp']=r.read(actor+0x5c,1)[0]
        new=set(sessions.glob('session-*'))-previous;assert len(new)==1,new
        folder=new.pop();trial['session']=folder.name
        trial['inputs']=[[int(v)for v in line[2:].split(',')]
            for line in(folder/'inputs.csv').read_text().splitlines()if line.startswith('N ')]
        trial['functions']=r.request('fntrace_dump',count=8192)
        trial['health_writes']=r.request('wtrace_dump',count=4096,addr_lo=hex(actor+0x5c),addr_hi=hex(actor+0x5d))
        (s.output/'saved-slash-comparison.json').write_text(json.dumps(result,indent=2))
    result['control_qualification']=qualify_saved_inputs(result)
    (s.output/'saved-slash-comparison.json').write_text(json.dumps(result,indent=2))
    return result['control_qualification']
