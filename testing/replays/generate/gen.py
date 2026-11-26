#!/usr/bin/env python3
"""Generate test songs (MIDI charts) and scripted players for the Performous autoplay harness.

Usage: gen.py <outdir>
Creates <outdir>/songs/<song>/{song.ini,notes.mid} and <outdir>/<run>.json for each run.
"""
import json
import os
import random
import struct
import sys

DIV = 480          # ticks per beat
TEMPO = 500000     # 120 BPM -> 960 ticks per second
TPS = DIV * 1e6 / TEMPO

# Button ids (controllers-buttons.ii)
G_FRET = [0, 1, 2, 3, 4]
G_PICK_DOWN, G_PICK_UP, G_GODMODE, G_WHAMMY = 11, 10, 0xF00, 0xF01
D_KICK, D_RED, D_YELLOW, D_BLUE, D_GREEN = 0, 1, 2, 3, 4
D_TOM = {2: 0x102, 3: 0x103, 4: 0x104}
D_CYM = {2: 0x202, 3: 0x203, 4: 0x204}

BASE = {0: 60, 1: 60, 2: 72, 3: 84, 4: 96}  # Kids, Easy, Medium, Hard, Expert


def ticks(t):
    return int(round(t * TPS))


def varlen(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append(0x80 | (n & 0x7F))
        n >>= 7
    return bytes(reversed(out))


def track(name, notes=(), texts=(), tempo=False):
    """notes: (begin, end, pitch); texts: (time, text)"""
    ev = []
    if name:
        ev.append((0, 0, b'\xff\x03' + varlen(len(name)) + name.encode()))
    if tempo:
        ev.append((0, 0, b'\xff\x51\x03' + struct.pack('>I', TEMPO)[1:]))
    for t, text in texts:
        ev.append((ticks(t), 1, b'\xff\x01' + varlen(len(text)) + text.encode()))
    for b, e, p in notes:
        tb, te = ticks(b), ticks(e)
        if te <= tb:
            te = tb + 1
        ev.append((te, 0, bytes([0x80, p, 0])))   # note offs first at equal ticks
        ev.append((tb, 2, bytes([0x90, p, 100])))
    ev.sort(key=lambda x: (x[0], x[1]))
    data, last = b'', 0
    for t, _, raw in ev:
        data += varlen(t - last) + raw
        last = t
    data += varlen(0) + b'\xff\x2f\x00'
    return b'MTrk' + struct.pack('>I', len(data)) + data


def midi(tracks):
    return b'MThd' + struct.pack('>IHHH', 6, 1, len(tracks), DIV) + b''.join(tracks)


# ---------------------------------------------------------------------------------------------
# Charts. A chart is a list of chords: (time, end, frets) with frets a tuple of 0..4.

def guitar_expert(bre):
    rng = random.Random(1)
    chords = []
    t = 4.0
    cycle = [0, 1, 2, 3, 4, 3, 2, 1]
    i = 0
    # A: singles and chords, short notes (not tappable: they end too early)
    while t < 12.0:
        if i % 8 == 7:
            chords.append((t, t + 0.05, tuple(sorted({cycle[i % 8], (cycle[i % 8] + 2) % 5}))))
        else:
            chords.append((t, t + 0.05, (cycle[i % 8],)))
        t += 0.25
        i += 1
    # A2: tappable runs (long enough notes, different frets)
    while t < 16.0:
        chords.append((t, t + 0.2, (cycle[i % 8],)))
        t += 0.25
        i += 1
    # A3: repeated frets (not tappable) and three-note chords
    while t < 20.0:
        if i % 4 == 0:
            chords.append((t, t + 0.05, (0, 2, 4)))
        else:
            chords.append((t, t + 0.05, ((i // 4) % 5,)))
        t += 0.25
        i += 1
    # B: sustains, single and chords
    for k, frets in enumerate([(0,), (1, 3), (2,), (0, 4)]):
        chords.append((20.0 + 2 * k, 21.5 + 2 * k, frets))
    # C: solo, fast singles
    t = 28.0
    while t < 37.0:
        chords.append((t, t + 0.1, (rng.randrange(5),)))
        t += 0.2
    # D: after GodMode (pressed around 39 s)
    t = 40.0
    while t < 56.0:
        f = cycle[i % 8]
        chords.append((t, t + (0.6 if i % 16 == 0 else 0.05), (f,) if i % 6 else (f, (f + 1) % 5)))
        t += 0.25
        i += 1
    # E: after the fill
    t = 61.0
    end = 70.0 if bre else 72.0
    while t < end:
        chords.append((t, t + 0.05, (cycle[i % 8],)))
        t += 0.25
        i += 1
    if bre:
        chords += [(71.0, 71.05, (0,)), (72.0, 72.05, (2,))]   # Under the Big Rock Ending: free points
        chords.append((74.5, 75.0, (0, 2)))                    # Hitting the last chord ends the BRE
    return [(b, e, tuple(sorted(set(f)))) for b, e, f in chords]


def reduce(chords, level):
    """Easier difficulties: fewer notes and frets."""
    if level == 4:
        return chords
    maxfret = {0: 2, 1: 2, 2: 3, 3: 4}[level]
    step = {0: 4, 1: 2, 2: 2, 3: 1}[level]
    out = []
    for n, (b, e, f) in enumerate(chords):
        if e - b < 1.0 and n % step:
            continue
        frets = tuple(sorted({min(x, maxfret) for x in f}))
        if level < 3:
            frets = frets[:1] if level < 2 else frets[:2]
        out.append((b, e, frets))
    return out


def drums_expert(bre):
    chords = []
    t = 4.0
    n = 0
    end = 70.0 if bre else 72.0
    while t < end:
        if 56.0 <= t < 61.0 and n % 4:   # Drum fill: a few notes under it (free points)
            t += 0.25
            n += 1
            continue
        pads = set()
        if n % 4 == 0:
            pads.add(D_KICK)
        if n % 4 == 2:
            pads.add(D_RED)
        if 30.0 <= t < 38.0:   # Tom section
            pads.add([D_YELLOW, D_BLUE, D_GREEN][n % 3])
        elif n % 2 == 0:
            pads.add(D_YELLOW)  # Hi-hat
        if n % 16 == 15:
            pads.add(D_GREEN)   # Crash
        if 44.0 <= t < 48.0 and n % 2:
            pads.add(D_BLUE)    # Ride
        if pads:
            chords.append((t, t + 0.05, tuple(sorted(pads))))
        t += 0.25
        n += 1
    if bre:
        chords += [(71.0, 71.05, (D_KICK,)), (72.0, 72.05, (D_KICK, D_RED))]  # Under the Big Rock Ending
        chords.append((74.5, 74.55, (D_KICK, D_GREEN)))   # Hitting the last chord ends the BRE
    return chords


def drums_reduce(chords, level):
    if level == 4:
        return chords
    out = []
    for n, (b, e, f) in enumerate(chords):
        if level < 3 and n % 2:
            continue
        frets = f if level >= 2 else tuple(x for x in f if x != D_KICK) or f
        out.append((b, e, frets))
    return out


TOMS = [(110, 30.0, 38.0), (111, 30.0, 34.0), (112, 34.0, 38.0)]  # pitch, begin, end


def is_tom(pad, t):
    pitch = {D_YELLOW: 110, D_BLUE: 111, D_GREEN: 112}.get(pad)
    return any(p == pitch and b <= t < e for p, b, e in TOMS)


def chart_notes(chords, base):
    return [(b, e, base + f) for b, e, frets in chords for f in frets]


def make_song(outdir, name, bre):
    g = guitar_expert(bre)
    d = drums_expert(bre)
    fills = [(56.0, 60.0)] + ([(70.5, 74.0)] if bre else [])
    def guitar_track(trackname, shift):
        notes = []
        for level in (1, 2, 3, 4):
            chords = [(b, e, tuple(sorted({(x + shift) % 5 for x in f}))) for b, e, f in reduce(g, level)]
            notes += chart_notes(chords, BASE[level])
        notes.append((28.0, 37.0, 103))  # Solo
        for b, e in fills:
            notes += [(b, e, p) for p in range(120, 125)]
        return track(trackname, notes)
    dnotes = []
    for level in (1, 2, 3, 4):
        dnotes += chart_notes(drums_reduce(d, level), BASE[level])
    dnotes += [(b, e, p) for p, b, e in TOMS]
    for b, e in fills:
        dnotes += [(b, e, p) for p in range(120, 125)]
    texts = [(70.0, '[section big_rock_ending]')] if bre else []
    tracks = [track('', texts=[(0.0, '[section intro]')] + texts, tempo=True),
              guitar_track('PART GUITAR', 0), guitar_track('PART BASS', 1), track('PART DRUMS', dnotes)]
    folder = os.path.join(outdir, 'songs', name)
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, 'notes.mid'), 'wb') as f:
        f.write(midi(tracks))
    with open(os.path.join(folder, 'song.ini'), 'w') as f:
        f.write(f'[song]\nname = {name}\nartist = Autoplay\ndelay = 0\n')
    return g, d


# ---------------------------------------------------------------------------------------------
# Players. Each returns a list of [time, button, value] events.

class Script:
    def __init__(self):
        self.ev = []

    def add(self, t, button, value):
        if t > 0.5:
            self.ev.append([round(t, 4), button, value])

    def sorted(self):
        return sorted(self.ev, key=lambda e: e[0])


def guitar_player(chords, seed, sloppy, godmode_at=None, bre=None):
    rng = random.Random(seed)
    s = Script()
    held = set()

    def set_frets(t, frets):
        for f in sorted(held - set(frets)):
            s.add(t, G_FRET[f], 0.0)
            held.discard(f)
        for f in sorted(set(frets) - held):
            s.add(t, G_FRET[f], 1.0)
            held.add(f)

    for n, (b, e, frets) in enumerate(chords):
        jitter = rng.uniform(-0.025, 0.025)
        tappable_like = e - b >= 0.15 and len(frets) == 1 and e - b < 1.0
        if sloppy:
            r = rng.random()
            if r < 0.12:
                continue                                    # Missed note
            if r < 0.20:
                jitter = rng.choice([-1, 1]) * rng.uniform(0.07, 0.25)   # Early/late, sometimes too much
            if r < 0.27:
                frets = tuple(sorted({(frets[0] + 1) % 5}))   # Wrong fret
            elif r < 0.32:
                frets = tuple(sorted(set(frets) | {0}))       # Extra lower fret held (allowed for singles)
            elif r < 0.36:
                frets = tuple(sorted(set(frets) | {4}))       # Extra higher fret (wrong)
        t = b + jitter
        if tappable_like and n % 3 and not sloppy:
            # Hammer-on: press the fret without picking (keep lower frets as is)
            set_frets(t, frets)
            continue
        if sloppy and tappable_like and rng.random() < 0.5:
            set_frets(t, frets)                             # Hammer-on attempts, some will be refused
            continue
        set_frets(t - 0.04, frets)
        s.add(t, rng.choice([G_PICK_DOWN, G_PICK_UP]), 1.0)
        s.add(t + 0.03, G_PICK_DOWN, 0.0)
        if e - b >= 1.0:
            end = e if not sloppy or rng.random() < 0.5 else b + (e - b) * rng.uniform(0.2, 0.8)
            # Whammy during the hold
            w = b + 0.1
            while w < end - 0.05:
                s.add(w, G_WHAMMY, rng.uniform(-1.0, 1.0))
                w += 0.1
            s.add(end, G_WHAMMY, -1.0)
            for f in frets:
                s.add(end, G_FRET[f], 0.0)
                held.discard(f)
        if sloppy and rng.random() < 0.05:
            s.add(t + 0.1, G_PICK_DOWN, 1.0)                # Extra pick
            s.add(t + 0.12, G_PICK_DOWN, 0.0)
        if sloppy and rng.random() < 0.05 and held:
            f = max(held)
            s.add(t + 0.12, G_FRET[f], 0.0)                 # Pull-off attempt
            held.discard(f)
    set_frets(chords[-1][1] + 0.2, ())
    if godmode_at is not None:
        s.add(godmode_at, G_GODMODE, 1.0)
        s.add(godmode_at + 0.1, G_GODMODE, 0.0)
        s.add(godmode_at + 0.5, G_GODMODE, 1.0)             # Second press while active: ignored
        s.add(godmode_at + 0.6, G_GODMODE, 0.0)
    # Drum fill / Big Rock Ending: hit frets
    for fb, fe in ([(56.0, 60.0)] + ([bre] if bre else [])):
        t = fb + 0.05
        while t < fe:
            f = rng.randrange(5)
            s.add(t, G_FRET[f], 1.0)
            s.add(t + 0.05, G_FRET[f], 0.0)
            t += rng.uniform(0.08, 0.2)
    return s.sorted()


def drums_player(chords, seed, sloppy, promode, fills, fill_rate=9.0, finish_fill=True):
    rng = random.Random(seed)
    s = Script()

    def hit(t, button):
        s.add(t, button, rng.uniform(0.5, 1.0))
        s.add(t + 0.04, button, 0.0)

    for b, e, pads in chords:
        for pad in pads:
            jitter = rng.uniform(-0.03, 0.03)
            button = pad
            if pad in D_TOM:
                button = D_TOM[pad] if is_tom(pad, b) else D_CYM[pad]
                if not promode and rng.random() < 0.5:
                    button = pad
            if sloppy:
                r = rng.random()
                if r < 0.12:
                    continue
                if r < 0.2:
                    jitter = rng.choice([-1, 1]) * rng.uniform(0.07, 0.25)
                elif r < 0.27:
                    button = rng.choice([D_RED, D_YELLOW, D_BLUE, D_GREEN, D_KICK])
                elif r < 0.32 and pad in D_TOM:
                    button = D_CYM[pad] if is_tom(pad, b) else D_TOM[pad]   # Tom instead of cymbal
                elif r < 0.36:
                    hit(b + jitter + 0.02, button)                  # Flam / double hit
            hit(b + jitter, button)
    for fb, fe in fills:
        t = fb + 0.02
        while t < fe - 0.1:
            hit(t, rng.choice([D_RED, D_YELLOW, D_BLUE, D_KICK]))
            t += 1.0 / fill_rate
        if finish_fill:
            hit(fe - 0.02, D_GREEN)
    return s.sorted()


def lefty(events):
    """Play for lefty mode: the game swaps the pads back (GuitarGraph flips all but the kick)."""
    def flip(b):
        layer, num = b >> 8, b & 0xFF
        if (layer < 2 and 1 <= num <= 4) or (layer == 2 and 2 <= num <= 3):
            return layer << 8 | (5 - num)
        return b
    return [(t, flip(b), v) for t, b, v in events]


def main():
    out = sys.argv[1]
    runs = {}
    for song, bre in (('Autoplay A', False), ('Autoplay B', True)):
        g, d = make_song(out, song, bre)
        brefill = (70.5, 74.0) if bre else None
        fills = [(56.0, 60.0)] + ([brefill] if bre else [])
        shift1 = lambda ch: [(b, e, tuple(sorted({(x + 1) % 5 for x in f}))) for b, e, f in ch]
        players = []
        if not bre:
            players.append(dict(name='guitar-expert-good', drums=False, keyboard=True, track='Guitar', level=4,
                                events=guitar_player(g, 11, False, godmode_at=39.0)))
            players.append(dict(name='bass-medium-sloppy', drums=False, keyboard=False, track='Bass', level=2,
                                events=guitar_player(shift1(reduce(g, 2)), 12, True, godmode_at=39.0)))
            players.append(dict(name='drums-expert-good', drums=True, keyboard=True, track='', level=4,
                                events=drums_player(d, 13, False, True, fills)))
            players.append(dict(name='drums-hard-sloppy', drums=True, keyboard=False, track='', level=3,
                                events=drums_player(drums_reduce(d, 3), 14, True, True, fills, fill_rate=4.0)))
            config = {'game/drum_promode': True, 'game/instrument_miss_penalty': True, 'audio/controller_delay': 0.08}
        else:
            players.append(dict(name='guitar-hard-sloppy', drums=False, keyboard=False, track='Guitar', level=3,
                                events=guitar_player(reduce(g, 3), 21, True, godmode_at=39.0, bre=brefill)))
            players.append(dict(name='guitar-kids-sloppy', drums=False, keyboard=True, track='Guitar', level=0,
                                config={'game/instrument_miss_penalty': True},  # Kids lose half the points
                                events=guitar_player(reduce(g, 1), 22, True, godmode_at=39.0, bre=brefill)))
            players.append(dict(name='drums-expert-good-lefty', drums=True, keyboard=True, track='', level=4, lefty=True,
                                events=lefty(drums_player(d, 23, False, False, fills))))
            players.append(dict(name='drums-easy-sloppy', drums=True, keyboard=False, track='', level=1,
                                events=drums_player(drums_reduce(d, 1), 24, True, False, fills, finish_fill=False)))
            config = {'game/drum_promode': False, 'game/instrument_miss_penalty': False, 'audio/controller_delay': 0.0}
        runs[song] = dict(song=song, end=77.0, players=players, config=config)
    for song, spec in runs.items():
        with open(os.path.join(out, song.replace(' ', '_') + '.json'), 'w') as f:
            json.dump(spec, f)
        print(song, [(p['name'], len(p['events'])) for p in spec['players']])


if __name__ == '__main__':
    main()
