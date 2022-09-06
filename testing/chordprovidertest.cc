#include "common.hh"

#include "game/guitarchords/chordprovider.hh"

namespace {
	auto const pi2 = static_cast<float>(M_PI * 2.0);

	void fill(Analyzer& analyzer, std::set<float> frequencies) {
		auto wave = std::vector<float>(48000);

		for(auto n = 0; n < 48000; ++n) {
			auto const nf = static_cast<float>(n);
			auto s = 0.f;
			for(auto frequency : frequencies)
				s += 0.25f * sin(nf * frequency * pi2 / 48000.f);

			wave[n] = s;
		}

		analyzer.input(wave.begin(), wave.end());
	}
	void fill(Analyzer& analyzer) {
		fill(analyzer, std::set<float>{});
	}
	void fill(Analyzer& analyzer, float frequency) {
		fill(analyzer, std::set<float>{frequency});
	}
	void fill(Analyzer& analyzer, float frequency0, float frequency1) {
		fill(analyzer, std::set<float>{frequency0, frequency1});
	}
	void fill(Analyzer& analyzer, float frequency0, float frequency1, float frequency2) {
		fill(analyzer, std::set<float>{frequency0, frequency1, frequency2});
	}
	void fill(Analyzer& analyzer, float frequency0, float frequency1, float frequency2, float frequency3) {
		fill(analyzer, std::set<float>{frequency0, frequency1, frequency2, frequency3});
	}
	void fill(Analyzer& analyzer, float frequency0, float frequency1, float frequency2, float frequency3, float frequency4) {
		fill(analyzer, std::set<float>{frequency0, frequency1, frequency2, frequency3, frequency4});
	}
	void fill(Analyzer& analyzer, float frequency0, float frequency1, float frequency2, float frequency3, float frequency4, float frequency5) {
		fill(analyzer, std::set<float>{frequency0, frequency1, frequency2, frequency3, frequency4, frequency5});
	}
}

const float E = 82.41f;
const float A = 110.f;
const float d = 146.83f;
const float g = 196.0f;
const float b = 246.94f;
const float e = 329.63f;

const float E1 = 87.f;
const float A1 = 117.f;
const float d1 = 156.f;
const float g1 = 208.f;
const float b1 = 262.f;
const float e1 = 349.f;

const float E2 = 92.5f;
const float A2 = 123.f;
const float d2 = 165.f;
const float g2 = 220.f;
const float b2 = 277.f;
const float e2 = 370.f;

const float E3 = 98.f;
const float A3 = 131.f;
const float d3 = 175.f;
const float g3 = 233.f;
const float b3 = 294.f;
const float e3 = 392.f;

const float E4 = 104.f;
const float A4 = 139.f;
const float d4 = 185.f;
const float g4 = 247.f;
const float b4 = 311.f;
const float e4 = 415.f;

TEST(Unittest_ChordProvider, no_input) {
	auto&& analyzer = Analyzer(48000., "guitar");
	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, IsEmpty());
}

TEST(Unittest_ChordProvider, silence) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, IsEmpty());
}

TEST(Unittest_ChordProvider, A) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer, A);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	ASSERT_THAT(result, Not(IsEmpty()));
	EXPECT_THAT(*result.begin(), FloatNear(A, 2.0));
}

TEST(Unittest_ChordProvider, A2) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer, 220.f);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	ASSERT_THAT(result, Not(IsEmpty()));
	EXPECT_THAT(*result.begin(), FloatNear(220, 2.0));
}

TEST(Unittest_ChordProvider, A3) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer, 440.f);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	ASSERT_THAT(result, Not(IsEmpty()));
	EXPECT_THAT(*result.begin(), FloatNear(440, 2.0));
}

TEST(Unittest_ChordProvider, E) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer, E);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	ASSERT_THAT(result, Not(IsEmpty()));
	EXPECT_THAT(*result.begin(), FloatNear(E, 2.0));
}

TEST(Unittest_ChordProvider, d) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer, d);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	ASSERT_THAT(result, Not(IsEmpty()));
	EXPECT_THAT(*result.begin(), FloatNear(d, 2.0));
}

TEST(Unittest_ChordProvider, g) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer, g);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	ASSERT_THAT(result, Not(IsEmpty()));
	EXPECT_THAT(*result.begin(), FloatNear(g, 2.0));
}

TEST(Unittest_ChordProvider, b) {
	auto&& analyzer = Analyzer(48000., "guitar");

	fill(analyzer, b);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	ASSERT_THAT(result, Not(IsEmpty()));
	EXPECT_THAT(*result.begin(), FloatNear(b, 2.0));
}

TEST(Unittest_ChordProvider, A_E) {
	auto&& analyzer = Analyzer(48000., "guitar", 100);

	fill(analyzer, A, E);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, Contains(FloatNear(A, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(E, 2.0)));
}

TEST(Unittest_ChordProvider, A_b) {
	auto&& analyzer = Analyzer(48000., "guitar", 100);

	fill(analyzer, A, b);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, Contains(FloatNear(A, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(b, 2.0)));
}

TEST(Unittest_ChordProvider, A_E_b) {
	auto&& analyzer = Analyzer(48000., "guitar", 100);

	fill(analyzer, A, E, b);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, Contains(FloatNear(A, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(E, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(b, 2.0)));
}

TEST(Unittest_ChordProvider, chord_Em) {
	auto&& analyzer = Analyzer(48000., "guitar", 100);

	fill(analyzer, A1, d1, g, b, e);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, Contains(FloatNear(A1, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(d1, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(g, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(b, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(e, 2.0)));
}

TEST(Unittest_ChordProvider, chord_D) {
	auto&& analyzer = Analyzer(48000., "guitar", 100);

	fill(analyzer, d, g2, b3, e2);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, Contains(FloatNear(d, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(g2, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(b3, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(e2, 2.0)));
}

TEST(Unittest_ChordProvider, chord_Dm) {
	auto&& analyzer = Analyzer(48000., "guitar", 100);

	fill(analyzer, d, g2, b3, e1);

	auto chordprovider = ChordProvider(analyzer);
	auto const result = chordprovider.getChord().getFrequencies();

	EXPECT_THAT(result, Contains(FloatNear(d, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(g2, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(b3, 2.0)));
	EXPECT_THAT(result, Contains(FloatNear(e1, 2.0)));
}

