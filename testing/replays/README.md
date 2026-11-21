# Recorded games for the replay test

`gamereplaytest.cc` replays these games through `GuitarEngine` and checks that every frame ends
with the score, streak and star meter that the game recorded (see `GameRecorder`).

They were recorded with the gameplay code from before `GuitarEngine` existed (commit "Save a guitar
or drums game for a bug report from the pause menu", where `GuitarGraph` does everything), so they
check that moving the rules out of `GuitarGraph` kept them the same. Any later change of the rules
fails the test as well: re-record the games then, and explain the score changes in the commit.

## What they play

Two generated songs (`generate/gen.py`): guitar, bass and drums charts in four difficulties with
chords, tappable runs, sustains, a solo, drum fills, toms and cymbals. Song B ends with a Big Rock
Ending. Scripted players, one per game:

| Game | Plays |
|------|-------|
| A-guitar-expert-good | ±25 ms timing, hammer-ons and pull-offs, whammy on sustains, GodMode (and a second press while active) |
| A-bass-medium-sloppy | joystick; misses, early/late hits, wrong and extra frets, extra picks, released sustains |
| A-drums-expert-good | pro drums: toms and cymbals, drum fills that activate GodMode |
| A-drums-hard-sloppy | pro drums: misses, wrong pads, tom instead of cymbal, double hits, slow fills |
| B-guitar-hard-sloppy | no miss penalty, sloppy, Big Rock Ending |
| B-guitar-kids-sloppy | kids difficulty with the miss penalty (half the points), Big Rock Ending |
| B-drums-expert-good-lefty | lefty mode, not pro drums, Big Rock Ending |
| B-drums-easy-sloppy | easy, sloppy, an unfinished fill |

Song A uses an input delay of 80 ms and the miss penalty, song B neither (except for the kids game).
Notes under the drum fills and the Big Rock Ending give free points; the BRE bonus needs the last
chord, after the fill. Every game starts on the default difficulty and then changes it, like a
player would in the join menu.

## Recording them again

The games are played by the real game, without anyone at the controls:

1. Apply `generate/autoplay.patch` to the code to record with. It plays the scripted inputs as
   if they came from a keyboard or joystick, saves the report at the end and quits. It also slows the
   null sound device down to real time. This patch is for test runs only, never commit it.
2. Build Performous.
3. Run `generate/record.sh <performous binary>`. It needs python3, ffmpeg and Xvfb, and uses its
   own home, config and data folders (your own config is not touched) with no sound. It takes about
   1.5 minutes per game.

Rendering is software only (Xvfb), about 12 frames per second. The game judges hits at frame time,
so the timing is coarser than a real game's, which does not matter for the comparison.

The score of sustains follows an animated value (correctness) that moves with the wall-clock time
between reads. The recording has the wall-clock time of every frame and draw, but the engine reads it
a little later, so the replayed score may differ by about a millionth; streaks and star meter must
match exactly.
