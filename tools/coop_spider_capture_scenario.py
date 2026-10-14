"""Native hostile-web death regression; health/position setup uses offline cheats."""
import struct
import time


def spider_capture(s):
    cases = []
    for victim in (0, 1):
        partner = victim ^ 1
        s.load(1, 1, 5)
        s.r.fixture(5, 3)
        s.r.input(0xffdf, 1)  # Both players touch when that door option is active.
        s.door(0, f'web-door-{victim}')
        s.wait('web-fight', lambda row: not row['blocked'] and not row['warning'] and
               not row['freeze'] and all(p['state'] == 1 and p['action'] == 2
                                         for p in row['players']), 25, True)
        s.r.teleport(victim, 4666, 1467)
        s.r.teleport(partner, 4870, 1467)
        s.r.fixture(2, victim, 6)
        s.r.fixture(5, 1 << partner)
        caught = None
        died = False
        samples = []
        deadline = time.monotonic() + 35
        while time.monotonic() < deadline:
            bodies = s.r.players()
            raw = s.r.read(0x8013f328, 32 * 0x9c)
            webs = [(i, raw[i*0x9c:(i+1)*0x9c]) for i in range(32)
                    if raw[i*0x9c] and raw[i*0x9c+1] == 22]
            row = s.sample('web-owner')
            samples.append(dict(players=row['players'], capture=[b[0xba] for b in bodies],
                                webs=[(i, list(b[4:6]), struct.unpack_from('<I', b, 0x8c)[0])
                                      for i, b in webs]))
            if caught is None and bodies[victim][0xba]:
                captured = [i for i, b in webs if struct.unpack_from('<I', b, 0x8c)[0] == 3]
                if captured:
                    caught = captured[0]
                    before = row['players'][partner]
                    s.sample(f'web-caught-{victim}', True)
            if caught is not None:
                died |= row['players'][victim]['hp'] == 0
                survivor = row['players'][partner]
                assert not bodies[partner][0xba], ('Capture transferred', row)
                assert survivor['hp'] == before['hp'] and abs(survivor['x']-before['x']) < 2, row
                if died and not any(i == caught for i, _ in webs):
                    break
            time.sleep(.08)
        else:
            raise AssertionError(('Native web capture/death did not complete', victim, caught, died))
        s.sample(f'web-released-{victim}', True)
        cases.append(dict(victim=victim, samples=samples))
    return dict(cases=cases)
