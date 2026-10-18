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
