"""Native P2-led Jungle exit with a partner below the upper room.

Development fixtures move bodies to observed safe positions and protect only
P2 from nearby enemies. The original barrier, attacks, exit walk and section
loader perform the transition. Supports synchronized netplay fixtures.
"""
import time


def jungle_exit(scenario, first_dead=False):
    s = scenario
    s.load(1, 0, 4)
    s.r.teleport(0, 5400, 1467)
    s.r.fixture(5, 2)
    try:
        # Visit the vertical room's native camera triggers before the upper
        # exit. Teleporting straight past them leaves the old horizontal cap.
        for x, y in ((5768, 1027), (5750, 954), (5832, 786), (5864, 695),
                     (5780, 622), (5736, 512), (5737, 426)):
            s.r.teleport(1, x, y)
            frame = s.sample('jungle-ascent')['frame']
            s.wait('jungle-camera-catch-up', lambda row: row['frame'] >= frame + 24, 8)
        s.r.teleport(1, 6193, 426)
        if first_dead:
            s.r.fixture(2, 0, 0x80)
            s.wait('jungle-p1-dead', lambda row: row['players'][0]['state'] == 3, 12)
        before = s.sample('jungle-exit-before', True)
        assert before['players'][1]['hp'] > 0, before
        assert (before['players'][0]['hp'] == 0) == first_dead, before
        deadline = time.monotonic() + 40
        while time.monotonic() < deadline:
            row = s.sample('jungle-exit-walk')
            if row['section'] == 1:
                break
            if not first_dead:
                assert row['players'][0]['hp'] > 0, ('Hidden passenger killed', row)
            # Pulse ordinary attacks while advancing into the real barrier.
            s.r.input(0x7fdf if (row['frame'] // 12) % 2 else 0xffdf, 1)
            time.sleep(.04)
        else:
            raise TimeoutError('Native Jungle exit did not enter part 2')
        s.r.release()
        after = s.wait('jungle-part2-arrival', lambda row:
            row['stage'] == 1 and row['section'] == 1 and row['mode'][:2] == [6, 0]
            and row['players'][1]['state'] == 1 and row['players'][1]['action'] == 2
            and (first_dead or row['players'][0]['action'] == 2), 35)
        assert after['lives'] == before['lives'], (before, after)
        for seat, player in enumerate(after['players']):
            if seat == 0 and first_dead:
                assert player['hp'] == 0 and player['state'] == 3, after
            else:
                assert player['active'] and player['visible'] and player['hp'] > 0, after
        s.sample('jungle-exit-after', True)
        return dict(before=before, after=after, first_dead=first_dead)
    finally:
        s.r.release()
        s.r.fixture(5, 0)
