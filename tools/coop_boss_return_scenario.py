"""Native boss-fight return while the other player keeps moving.

Works with the isolated local or synchronized netplay Runtime. Fixtures only
choose the room, grant Ultimate Armor and prevent setup deaths; Select,
movement, jumping and Nova Strike use normal controller input.
"""
import struct
import time


def boss_return(s, capture=None):
    r = s.r
    r.fixture(5, 3)
    r.fixture(7, s.campaign, 0, 2)
    s.load(11, 0, 1)
    kind = 66 if s.campaign else 73
    r.input(0xffdf, 0)
    try:
        s.wait('boss-return-fight', lambda _: any(e['type'] == kind and e['hp']
               for e in s.enemies()), 30, True)
    finally:
        r.release()
    r.wait_playable(30, advance_dialogue=True)
    cases = []

    def sample(label):
        mirror = r.read(r.diagnostic(), 0x464)
        row = dict(label=label, frame=struct.unpack('<I', r.read(0x80141bd8, 4))[0],
                   players=[mirror[at:at + 0xe4].hex() for at in (0x300, 0x100)],
                   play=mirror[0x400:].hex())
        s.events.append(row)
        return row

    def wait(label, predicate, seconds=10):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            row = sample(label)
            if predicate(row):
                return row
            time.sleep(.015)
        raise AssertionError((label, row))

    def advance(ticks):
        start = sample('advance')['frame']
        wait('advance', lambda row: row['frame'] >= start + ticks)

    for name, buttons in (('walk', 0xffdf), ('dash', 0xdf7f),
                          ('jump', 0xbfff), ('nova', 0xfdff)):
        # Zero has no Nova; both rosters still exercise the jumping failure.
        if name == 'nova' and s.campaign:
            continue
        r.release()
        # The native Nova admission rejects an attack into a solid wall.
        # Reset to the observed arena floor, facing inward, between cases.
        r.teleport(0, 2200, 442 if s.campaign else 443)
        r.input(0xffdf, 0)
        advance(4)
        r.release()
        wait('leader-grounded', lambda row: bytes.fromhex(row['players'][0])[0x89] & 8)
        r.input(0xfffe, 1)
        wait('partner-withdrawn', lambda row: not bytes.fromhex(row['players'][1])[0])
        r.release()
        advance(4)
        r.input(0xfffe, 1)
        incoming = wait('partner-incoming', lambda row:
                        bytes.fromhex(row['players'][1])[0] and
                        bytes.fromhex(row['players'][1])[4:6] == bytes([1, 0]))
        r.release()
        r.input(buttons, 0)
        if capture:
            capture('boss-return-' + name + '-start')
        actions = set()
        try:
            def completed(row):
                first, second = [bytes.fromhex(p) for p in row['players']]
                actions.add(first[5])
                assert second[0], ('Arrival canceled by leader action', name, row)
                assert second[0x5c] & 127, ('Returning partner died', name, row)
                return second[4:6] == bytes([1, 2])
            done = wait('boss-return-' + name, completed)
            expected = dict(walk={3, 4}, dash={12, 13}, jump={6, 7}, nova={34})[name]
            assert actions & expected, ('Leader action was not exercised', name, actions)
            assert done['frame'] - incoming['frame'] < 90, ('Arrival stalled', name)
            if capture:
                capture('boss-return-' + name + '-complete')
            cases.append(dict(action=name, frames=done['frame'] - incoming['frame'],
                              leader_actions=sorted(actions), uninterrupted=True))
        finally:
            r.release()
        advance(70)
    return dict(boss=kind, campaign=s.campaign, cases=cases)
