#include "common.hh"

#include "game/guitarengine.hh"

#include <initializer_list>
#include <vector>

// GuitarEngine driven by timestamped input events, without graphics, audio or config.

namespace {
	using Difficulty = GuitarEngine::Difficulty;
	const unsigned expert = 0x60;  // Base MIDI pitch of the expert notes, + fret

	/// AnimValue (correctness, GodMode...) reads this clock, so tests control how animations progress
	Time fakeNow;
	Time fakeClock() { return fakeNow; }

	struct Note { unsigned fret; double begin, end; };

	InstrumentTrack makeTrack(std::initializer_list<Note> notes) {
		InstrumentTrack track("guitar");
		for (auto const& n: notes) track.nm[expert + n.fret].push_back(Duration(n.begin, n.end));
		return track;
	}

	struct Recorder: GuitarEngine::Listener {
		std::vector<int> misses;
		std::vector<unsigned> flames;
		unsigned passed = 0;
		void onMiss(int fret) override { misses.push_back(fret); }
		void onFlame(unsigned fret) override { flames.push_back(fret); }
		void onChordPassed(double) override { ++passed; }
	};

	struct FakeClock {
		FakeClock() { animclock::now = &fakeClock; }
		~FakeClock() { animclock::now = &animclock::realNow; }
	};

	/// Plays the role of GuitarGraph::engine(): maps buttons to held frets and calls the engine
	class Player {
		FakeClock m_clock;  // Before the engine, whose AnimValues read the clock when constructed
	  public:
		Player(InstrumentTrack const& track, bool drums = false, Difficulty level = Difficulty::AMAZING): engine(listener, drums, false, false), m_drums(drums) {
			engine.settings.missPenalty = true;
			EXPECT_TRUE(engine.setDifficulty(track, level));
		}
		/// Run one frame at song time t with the given events (all stamped with t)
		void frame(double t, std::initializer_list<std::pair<input::ButtonId, double>> events = {}) {
			fakeNow += clockDur(Seconds(t - m_last > 0.0 ? t - m_last : 0.0));
			m_last = t;
			engine.beginFrame(t);
			for (auto const& e: events) {
				input::Event ev;
				ev.button = input::Button(e.first);
				ev.value = e.second;
				unsigned num = ev.button.num();
				bool pick = ev.button == input::ButtonId::GUITAR_PICK_DOWN || ev.button == input::ButtonId::GUITAR_PICK_UP;
				if (!m_drums && !pick && ev.button.layer() == 0 && num + 1 < max_panels) engine.setPressed(num + 1, ev.pressed());
				engine.input(t, ev);
			}
			engine.update(t);
		}
		void hold(double t, input::ButtonId fret) { frame(t, {{fret, 1.0}}); }
		void release(double t, input::ButtonId fret) { frame(t, {{fret, 0.0}}); }
		void pick(double t) { frame(t, {{input::ButtonId::GUITAR_PICK_DOWN, 1.0}, {input::ButtonId::GUITAR_PICK_DOWN, 0.0}}); }
		void hit(double t, input::ButtonId pad) { frame(t, {{pad, 1.0}}); }

		Recorder listener;
		GuitarEngine engine;
	  private:
		bool m_drums;
		double m_last = 0.0;
	};

	using B = input::ButtonId;
}

TEST(GuitarEngine, BuildsChordsFromSimultaneousNotes) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 1.0, 1.0}, {2, 2.0, 2.0}});
	Player p(track);
	auto const& chords = p.engine.chords();
	ASSERT_EQ(chords.size(), 2u);
	EXPECT_EQ(chords[0].polyphony, 2u);
	EXPECT_TRUE(chords[0].fret[0] && chords[0].fret[1]);
	EXPECT_EQ(chords[1].polyphony, 1u);
	EXPECT_DOUBLE_EQ(p.engine.getNotesBeginTime(), 1.0);
}

TEST(GuitarEngine, UnavailableDifficultyIsRejected) {
	auto track = makeTrack({{0, 1.0, 1.0}, {0, 2.0, 2.0}});
	EXPECT_TRUE(GuitarEngine::hasDifficulty(track, Difficulty::AMAZING));
	EXPECT_FALSE(GuitarEngine::hasDifficulty(track, Difficulty::MEDIUM));
}

TEST(GuitarEngine, PickOnTimeScoresFullPoints) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 2.0, 2.0}});
	Player p(track);
	p.hold(0.9, B::GUITAR_GREEN);
	p.pick(1.0);
	EXPECT_EQ(p.engine.streak(), 1);
	EXPECT_DOUBLE_EQ(p.engine.score(), 50.0);
	EXPECT_EQ(p.engine.chords()[0].status, 2u);  // picked
	EXPECT_TRUE(p.listener.misses.empty());
}

TEST(GuitarEngine, LatePickScoresLess) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 2.0, 2.0}});
	Player p(track);
	p.hold(0.9, B::GUITAR_GREEN);
	p.pick(1.08);  // 80 ms late
	EXPECT_DOUBLE_EQ(p.engine.score(), 30.0);
}

TEST(GuitarEngine, ChordScoresPerNote) {
	auto track = makeTrack({{0, 1.0, 1.0}, {2, 1.0, 1.0}, {1, 2.0, 2.0}});
	Player p(track);
	p.frame(0.9, {{B::GUITAR_GREEN, 1.0}, {B::GUITAR_YELLOW, 1.0}});
	p.pick(1.0);
	EXPECT_DOUBLE_EQ(p.engine.score(), 100.0);
}

TEST(GuitarEngine, PickWithWrongFretIsAMiss) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 2.0, 2.0}});
	Player p(track);
	p.hold(0.9, B::GUITAR_RED);
	p.pick(1.0);
	EXPECT_EQ(p.engine.streak(), 0);
	ASSERT_EQ(p.listener.misses.size(), 1u);
	EXPECT_EQ(p.listener.misses[0], -1);  // A pick miss plays any fail sample
	EXPECT_DOUBLE_EQ(p.engine.score(), 0.0);  // The miss penalty never makes the score negative
}

TEST(GuitarEngine, UnplayedChordIsPassedAndBreaksTheStreak) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 2.0, 2.0}, {2, 3.0, 3.0}});
	Player p(track);
	p.hold(0.9, B::GUITAR_GREEN);
	p.pick(1.0);
	EXPECT_EQ(p.engine.streak(), 1);
	p.frame(2.0 + GuitarEngine::maxTolerance + 0.01);
	EXPECT_EQ(p.engine.streak(), 0);
	EXPECT_EQ(p.listener.passed, 2u);  // The first chord (played) and the second one (missed)
}

TEST(GuitarEngine, LowerFretsMayBeHeldForSingleNotes) {
	// A single note only requires the frets right of it to match: lower frets may be held too
	auto track = makeTrack({{2, 1.0, 1.0}, {1, 2.0, 2.0}});
	Player p(track);
	p.frame(0.9, {{B::GUITAR_GREEN, 1.0}, {B::GUITAR_YELLOW, 1.0}});
	p.pick(1.0);
	EXPECT_EQ(p.engine.streak(), 1);
}

TEST(GuitarEngine, HammerOnHitsTappableNote) {
	// Red follows green quickly, so it is tappable
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 1.1, 1.1}, {2, 3.0, 3.0}});
	Player p(track);
	ASSERT_TRUE(p.engine.chords()[1].tappable);
	p.hold(0.9, B::GUITAR_GREEN);
	p.pick(1.0);
	p.hold(1.1, B::GUITAR_RED);  // Hammer-on: press without picking
	EXPECT_EQ(p.engine.streak(), 2);
	EXPECT_EQ(p.engine.chords()[1].status, 1u);  // tapped
}

TEST(GuitarEngine, PullOffHitsTappableNote) {
	// Red then green: releasing red while holding green plays the green note
	auto track = makeTrack({{1, 1.0, 1.0}, {0, 1.1, 1.1}, {2, 3.0, 3.0}});
	Player p(track);
	ASSERT_TRUE(p.engine.chords()[1].tappable);
	p.frame(0.9, {{B::GUITAR_GREEN, 1.0}, {B::GUITAR_RED, 1.0}});
	p.pick(1.0);
	ASSERT_EQ(p.engine.streak(), 1);
	p.release(1.1, B::GUITAR_RED);
	EXPECT_EQ(p.engine.streak(), 2);
	EXPECT_EQ(p.engine.chords()[1].status, 1u);
}

TEST(GuitarEngine, NoHammerOnAfterAMiss) {
	// After a miss, correctness drops to the miss volume (0 here), which disables tapping
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 1.1, 1.1}, {2, 3.0, 3.0}});
	Player p(track);
	p.hold(0.9, B::GUITAR_YELLOW);
	p.pick(1.0);  // miss
	p.release(1.05, B::GUITAR_YELLOW);
	p.hold(1.1, B::GUITAR_RED);
	EXPECT_EQ(p.engine.streak(), 0);
	EXPECT_EQ(p.engine.chords()[1].status, 0u);
}

TEST(GuitarEngine, HoldingASustainScoresOverTime) {
	auto track = makeTrack({{0, 1.0, 3.0}, {1, 4.0, 4.0}});
	Player p(track);
	p.hold(0.9, B::GUITAR_GREEN);
	p.pick(1.0);
	double const afterPick = p.engine.score();
	for (double t = 1.1; t <= 2.0 + 1e-9; t += 0.1) p.frame(t);
	EXPECT_NEAR(p.engine.score() - afterPick, 50.0, 1e-6);  // 50 points per second held
	p.release(2.0, B::GUITAR_GREEN);
	p.frame(2.5);
	EXPECT_NEAR(p.engine.score() - afterPick, 50.0, 1e-6);  // Nothing more once released
}

TEST(GuitarEngine, HoldingTheLastChordToTheEndIsSafe) {
	// m_chordIt reaches end() while the last sustain is still held (issue #768)
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 2.0, 4.0}});
	Player p(track);
	p.hold(1.9, B::GUITAR_RED);
	p.pick(2.0);
	for (double t = 2.1; t < 4.5; t += 0.1) p.frame(t);
	EXPECT_NEAR(p.engine.score(), 50.0 + 2.0 * 50.0, 1e-6);
}

TEST(GuitarEngine, LookaheadMarksChordsPassed) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 2.0, 2.0}});
	Player p(track);
	p.engine.updateLookahead(1.5);
	EXPECT_TRUE(p.engine.chords()[0].passed);
	EXPECT_FALSE(p.engine.chords()[1].passed);
}

TEST(GuitarEngine, DrumHitOnTime) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 1.0, 1.0}, {2, 2.0, 2.0}});
	Player p(track, true);
	p.hit(1.0, B::DRUMS_KICK);
	EXPECT_EQ(p.engine.streak(), 0);  // Chord not complete yet
	p.hit(1.01, B::DRUMS_RED);
	EXPECT_EQ(p.engine.streak(), 1);
	EXPECT_EQ(p.engine.chords()[0].status, 2u);
}

TEST(GuitarEngine, WrongDrumPadIsPenalisedOnce) {
	auto track = makeTrack({{0, 1.0, 1.0}, {1, 2.0, 2.0}});
	Player p(track, true);
	p.hit(1.0, B::DRUMS_KICK);
	ASSERT_DOUBLE_EQ(p.engine.score(), 50.0);
	p.hit(1.5, B::DRUMS_YELLOW);
	ASSERT_EQ(p.listener.misses.size(), 1u);
	EXPECT_EQ(p.listener.misses[0], 2);  // The pad's own fail sample
	EXPECT_DOUBLE_EQ(p.engine.score(), 0.0);  // One penalty of 50 points
}

TEST(GuitarEngine, KidsModeAcceptsAnyDrumPad) {
	InstrumentTrack track("drums");
	unsigned const kids = 0x3C;
	track.nm[kids + 1].push_back(Duration(1.0, 1.0));
	track.nm[kids + 1].push_back(Duration(2.0, 2.0));
	Player p(track, true, Difficulty::KIDS);
	p.hit(1.0, B::DRUMS_GREEN);
	EXPECT_EQ(p.engine.streak(), 1);
	EXPECT_TRUE(p.listener.misses.empty());
}

TEST(GuitarEngine, CurrentBehaviour_FirstWrongDrumPadOnTimeCountsAsAHit) {
	// FIXME: characterisation of a bug found while extracting the engine. A pad that is not in the
	// chord has dur == nullptr, and m_notes[nullptr] is 0 until the first such hit, so the first
	// wrong pad within the tolerance is scored as a hit (and later ones are rejected).
	auto track = makeTrack({{0, 1.0, 1.0}, {0, 2.0, 2.0}, {0, 3.0, 3.0}});
	Player p(track, true);
	p.hit(1.0, B::DRUMS_YELLOW);
	EXPECT_TRUE(p.listener.misses.empty());
	EXPECT_EQ(p.engine.streak(), 1);
	p.hit(2.0, B::DRUMS_YELLOW);
	EXPECT_EQ(p.listener.misses.size(), 1u);
}
