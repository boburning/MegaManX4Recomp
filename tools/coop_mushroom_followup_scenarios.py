"""Native checks for the renewed Playtest40 object and rainbow reports."""
import json
import struct
import time


def intro_first_drop(s):
    r = s.r
    s.load(0, 1)
    r.fixture(5, 3)
    before = s.wait('intro-first-drop-ready', lambda v: not v['blocked'] and all(
        p['hp'] == 32 and p['state'] == 1 and p['action'] == 2
        for p in v['players']), 15)
    s.sample('intro-first-drop-before', True)
    try:
        r.input(0xffdf, 1); time.sleep(.8)
        deadline = time.monotonic() + 14
        while time.monotonic() < deadline:
            # Original wall jumps: no object/tile/trigger or collision edits.
            r.input(0xbfdf, 1); time.sleep(.08)
            r.input(0xffdf, 1); time.sleep(.035)
            player = r.players()[1]
            assert player[0x5c] & 127, 'P2 died during the native wall approach'
            if struct.unpack_from('<i', player, 8)[0] >= 624 << 16:
                break
        else:
            raise TimeoutError('P2 did not cross the original first drop threshold')
    finally:
        r.release()

    def lower_piece():
        pool = r.read(0x80165a30, 32*0x8c)
        for slot in range(32):
            b = pool[slot*0x8c:(slot+1)*0x8c]
            if b[0] and b[1:3] == bytes([1, 14]):
                return dict(slot=slot, state=list(b[4:7]),
                    x=struct.unpack_from('<i', b, 8)[0]/65536,
                    y=struct.unpack_from('<i', b, 12)[0]/65536)

    s.wait('intro-lower-piece-dropped', lambda _: (p := lower_piece()) and
           p['state'][:2] == [1, 3] and 392 <= p['y'] < 393, 5)
    after = s.sample('intro-first-drop-p2-only', True)
    assert abs(after['players'][0]['x'] - before['players'][0]['x']) < 2
    assert after['players'][1]['x'] >= 624 and all(p['hp'] == 32 for p in after['players'])
    assert after['lives'] == before['lives']
    return dict(before=before, after=after, lower_piece=lower_piece())


def mushroom_survivor_colors(s):
    r = s.r
    # Acquired-weapon fixtures also affect defeated-boss routing. This is a
    # private original first encounter, with P1 dying before initialization.
    r.write(0x80172219, 0)
    s.load(3, 1, 3)
    r.fixture(5, 3)
    r.fixture(2, 0, 0x80)
    s.wait('mushroom-p1-dead-before-entry', lambda v: v['players'][0]['state'] == 3, 15)
    colors, palette = struct.unpack('<2I', r.read(0x1f800024, 8))
    r.teleport(1, 6048, 450); time.sleep(.8)
    r.input(0xff7f, 1); time.sleep(2); r.release()
    try:
        r.input(0xffdf, 1)
        s.wait('mushroom-survivor-native-admission',
               lambda v: v['warning'] or v['blocked'], 15)
    finally:
        r.release()
    fight = s.wait('mushroom-survivor-live', lambda v: not v['warning'] and
        not v['blocked'] and not v['freeze'] and any(
            e['type'] == 61 and e['hp'] > 1 for e in s.enemies()), 40, confirm=True)
    boss = next(e for e in s.enemies() if e['type'] == 61 and e['hp'] > 1)
    r.fixture(6, boss['slot'], 12)  # Permit the native low-health copy pattern.
    observations = []
    try:
        for i in range(30):
            time.sleep(.35)
            row = s.sample('mushroom-survivor-pattern-%02d' % i, i % 5 == 0)
            row['bosses'] = [e for e in s.enemies() if e['type'] == 61]
            observations.append(row)
            assert row['players'][0]['hp'] == 0 and row['players'][1]['hp'] == 32
    finally:
        (s.output/'mushroom-survivor-phases.json').write_text(json.dumps(observations, indent=2)+'\n')
    source = r.read(colors + 0x6a0, 128)
    destination = r.read(palette + 0x780, 128)
    assert any(source) and destination == source, 'Native rainbow palette was lost in survivor projection'
    return dict(fight=fight, colors=hex(colors), palette=hex(palette),
                original_colors=source.hex(), working_colors=destination.hex(),
                phases=sorted(set(tuple(e['state']) for row in observations for e in row['bosses'])))
