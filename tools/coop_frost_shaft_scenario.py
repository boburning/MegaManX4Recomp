"""Bounded native rising-platform camera check at Frost part1 checkpoint3.

Stage/position/incoming-damage fixtures prepare the encounter. Native terrain,
platforms, camera updates and deaths remain intact. Stop during the ascent,
before walking into the authentic destructible ceiling; this is not a full
stage completion test. The same scenario supports synchronized netplay setup.
"""
import json
import struct
import time


def frost_shaft(s, capture=None):
    r = s.r
    state = next(c['count'] for c in r.request('mod_counters')['counters']
                 if c['name'] == 'mmx4.coop.split-state')

    def sample(label):
        row = s.sample(label)
        cameras = r.read(state + 16, 2 * 0xFC)
        row['private_cameras'] = [dict(
            x=struct.unpack_from('<i', cameras, seat * 0xFC + 8)[0] / 65536,
            y=struct.unpack_from('<i', cameras, seat * 0xFC + 12)[0] / 65536,
            limits=list(struct.unpack_from('<8h', cameras, seat * 0xFC + 0x1C)),
            follow=list(struct.unpack_from('<4h', cameras, seat * 0xFC + 0x2C)),
        ) for seat in (0, 1)]
        return row

    cases = []
    try:
        for leader, first_dead in ((0, False), (1, False), (1, True)):
            s.load(2, 0, 3)
            r.fixture(5, 3)
            if first_dead:
                r.fixture(2, 0, 0x80)
                s.wait('shaft-p1-dead', lambda row: row['players'][0]['state'] == 3, 15)
                r.teleport(1, 5280, 1354)
            else:
                r.teleport_both(5260, 1354)
                r.teleport(leader, 5280, 1354)
            s.wait('shaft-grounded', lambda row: all(
                p['hp'] and p['grounded'] for seat, p in enumerate(row['players'])
                if not first_dead or seat == 1), 10)
            case = dict(leader=leader, first_dead=first_dead, rows=[])
            cases.append(case)
            ascent_start = None
            deadline = time.monotonic() + 30
            try:
                for seat in ((1,) if first_dead else (0, 1)):
                    r.input(0xFFDF, seat)
                while time.monotonic() < deadline:
                    row = sample('shaft-ascent')
                    case['rows'].append(row)
                    player = row['players'][1]
                    assert player['hp'], ('Rider died before bounded ascent completed', row)
                    if player['x'] > 5500 and player['grounded']:
                        if ascent_start is None or player['y'] > ascent_start['players'][1]['y']:
                            ascent_start = row
                        # The native96..112 dead zone permits16 pixels of
                        # rider motion before the camera follows. Stop as
                        # soon as a healthy rider and camera both ascend;
                        # platform phase must not turn this into a ceiling test.
                        if (player['y'] < ascent_start['players'][1]['y'] and
                            ascent_start['private_cameras'][1]['y'] -
                                row['private_cameras'][1]['y'] >= 4):
                            break
                    time.sleep(.07)
                else:
                    raise TimeoutError('No native rising-platform ascent observed')
            finally:
                r.release()
            camera = row['private_cameras'][1]
            assert ascent_start, case
            assert ascent_start['private_cameras'][1]['y'] - camera['y'] >= 4, case
            assert 90 <= player['y'] - camera['y'] <= 120, case
            assert camera['limits'][0] == camera['limits'][4] == 5728, case
            if not first_dead:
                assert row['players'][0]['hp'], row
            if capture:
                capture('shaft-leader%d-dead%d' % (leader, first_dead))
            case['qualified'] = True
    finally:
        (s.output / 'frost-shaft.json').write_text(json.dumps(cases, indent=2))
    return dict(native_ascent_cases=3, landing_limit=True, independent_follow=True,
                complete_level=False)
