"""Native co-op boss damage, cooldown and death qualification."""
import time


def boss_balance(s, spider=False):
    def counts():
        return {c['name']: c['count'] for c in s.r.request('mod_counters')['counters']}

    s.load(1 if spider else 5, 1, 5 if spider else 1)
    if not spider:
        height = int(s.sample('balance-setup')['players'][0]['y'])
        s.r.teleport_both(16848, height)
        s.r.teleport(0, 16936, height)
    s.door(0, 'balance-door')
    s.wait('balance-live', lambda r: not r['blocked'] and not r['warning'] and
           not r['freeze'] and all(p['active'] and p['visible'] and p['state'] == 1 and
                                 p['action'] == 2 for p in r['players']), 25, True)
    boss = next(e for e in s.enemies() if e['type'] == (43 if spider else 56) and e['hp'] == 48)
    bounds = s.sample('balance-bounds')['bounds']
    address = 0x8013bed0 + boss['slot'] * 0x9c
    original_table = s.r.read(int(boss['weakness'], 16), 128).hex()
    initial = counts()
    samples = []

    def attack(seat):
        target = next(e for e in s.enemies() if e['slot'] == boss['slot'])
        x = max(bounds[1]+48, min(bounds[0]+288, int(target['x'])-48)) if spider else max(17088, min(17296, int(target['x']) - 48))
        y = max(bounds[3]+32, min(bounds[2]+220, int(target['y'])+24)) if spider else max(160, min(283, int(target['y']) + 24))
        s.r.teleport(seat, x, y)
        s.r.input(0xff7f if target['x'] < x else 0xffdf, seat)
        time.sleep(.025)
        s.r.release()
        s.r.input(0x7fff, seat)
        time.sleep(1.4 if (s.campaign ^ seat) == 0 else .09)
        s.r.release()
        deadline = time.monotonic() + .65
        while time.monotonic() < deadline:
            body = s.r.read(address, 0x9c)
            samples.append(dict(seat=seat, hp=body[0x5c], cooldown=body[0x8a],
                                state=list(body[4:7])))
            time.sleep(.025)

    for seat in (0, 1):
        hp_before = s.r.read(address + 0x5c, 1)[0]
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            attack(seat)
            hp = s.r.read(address + 0x5c, 1)[0]
            if hp < hp_before:
                break
        else:
            raise AssertionError(('Native boss was not damaged', seat, samples[-10:]))
        s.sample('balance-native-hit-' + str(seat), True)
    if s.boss_scaling and getattr(s,'boss_cooldowns','per_player')=='per_player':
        overlap='mmx4.coop.boss-hit-during-partner-cooldown'
        overlap_before=counts().get(overlap,0)
        for attempt in range(16):
            target=next(e for e in s.enemies() if e['slot']==boss['slot'])
            x=max(bounds[1]+48,min(bounds[0]+288,int(target['x'])-32)) if spider else max(17088,min(17296,int(target['x'])-32))
            y=max(bounds[3]+32,min(bounds[2]+220,int(target['y'])+24)) if spider else max(160,min(283,int(target['y'])+24))
            s.r.teleport_both(x,y)
            for seat in (0,1):s.r.input(0xff7f if target['x']<x else 0xffdf,seat)
            time.sleep(.03);s.r.release()
            for seat in (0,1):s.r.input(0x7fff,seat)
            time.sleep(.16);s.r.release();time.sleep(.08)
            if counts().get(overlap,0)>overlap_before:break
        else:raise AssertionError('No native damage accepted during the partner cooldown')
        s.sample('balance-independent-overlap',True)
    final = counts()
    hp = s.r.read(address + 0x5c, 1)[0]
    if s.boss_scaling:
        raw = final['mmx4.coop.boss-damage-before-scaling'] - initial.get('mmx4.coop.boss-damage-before-scaling', 0)
        applied = final['mmx4.coop.boss-damage-after-scaling'] - initial.get('mmx4.coop.boss-damage-after-scaling', 0)
        zero = final.get('mmx4.coop.zero-boss-damage-before-scaling',0) - initial.get('mmx4.coop.zero-boss-damage-before-scaling',0)
        assert applied == (raw*5-zero)//10 and hp == 48 - applied, (raw, zero, applied, hp)
        separate=getattr(s,'boss_cooldowns','per_player')=='per_player'
        counter='mmx4.coop.per-player-boss-cooldown' if separate else 'mmx4.coop.shorter-boss-cooldown'
        assert final[counter] > initial.get(counter, 0)
        assert max(r['cooldown'] for r in samples) <= 32, samples
    else:
        assert not final.get('mmx4.coop.scaled-boss-hit') and hp < 48, (hp, final)
    # Exercise real native death after the fixture sets up its final HP.
    s.r.fixture(6, boss['slot'], 1)
    deadline = time.monotonic() + 30
    while time.monotonic() < deadline:
        attack(1)
        body = s.r.read(address, 0x9c)
        if body[4] == 2 and body[0x5c] == 0:
            break
    else:
        raise AssertionError('Scaled boss failed to enter native death')
    return dict(scaled=s.boss_scaling, campaign=s.campaign, hp_after_both_players=hp,
                original_weakness_table=original_table, native_samples=samples,
                counts_before=initial, counts_after=final, native_death=True)
