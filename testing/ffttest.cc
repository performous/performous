#include "common.hh"

#include "game/guitarchords/fft.hh"
#include "game/guitarchords/chord.hh"
#include "game/guitarchords/chords.hh"
#include "game/guitarchords/notes.hh"


struct UnitTest_FFT : public testing::Test {
	float const pi2 = static_cast<float>(M_PI * 2.0);
	std::vector<String> const strings{E0, a0, d0, g0, b0, e0};

	float makeWave(float n, float frequency) {
		return sin(n * frequency * pi2 / 48000.f);
	}
	void fill(std::vector<float>& data, std::set<StringPlay> const& plays) {
		for(auto n = 0; n < data.size(); ++n) {
			auto const nf = static_cast<float>(n);
			auto s = 0.f;
			auto i = 0;
			for(auto const& play : plays) {
				s += 0.25f * makeWave(nf, play.getFrequency());
				s += 0.05f * makeWave(nf, play.getFrequency(true));
			}

			data[n] = s;
		}
	}

	void fill(std::vector<float>& data, std::vector<Fret> const& frets) {
		for(auto n = 0; n < data.size(); ++n) {
			auto const nf = static_cast<float>(n);
			auto s = 0.f;
			auto i = 0;
			for(auto fret : frets) {
				s += 0.25f * makeWave(nf, strings[i++].getFrequency(fret));
				s += 0.05f * makeWave(nf, strings[i++].getFrequency(fret, true));
			}

			data[n] = s;
		}
	}

	void printBest(std::vector<FFTItem> const& result, unsigned count = 10) {
		auto sorted = std::map<float, float>();

		for(auto const& item : result) {
			//std::cout << item.frequency << ": " << item.power << std::endl;
			sorted[-item.power] = item.frequency;
		}

		auto n = 0;
		for(auto const& item : sorted) {
			std::cout << item.second << ": " << item.first << std::endl;
			if(++n == count)
				break;
		}
	}
	void check(std::vector<FFTItem> const& result, std::set<StringPlay> const& targets) {
		auto frequencies = std::set<float>();

		for(auto const& item : result)
			if(item.power > 0.1)
				frequencies.insert(item.frequency);

		auto const binWidth = 48000.f / 8192.f;

		for(auto const& target : targets) {
			EXPECT_THAT(frequencies, Contains(FloatNear(target.getFrequency(), binWidth)));
			if(target.hasInverseFrequency())
				EXPECT_THAT(frequencies, Contains(FloatNear(target.getFrequency(true), binWidth)));
		}
	}

	void check(std::vector<FFTItem> const& result, Chord const& chord) {
		auto frequencies = std::set<float>();

		for(auto const& item : result)
			if(item.power > 0.1)
				frequencies.insert(item.frequency);

		auto const binWidth = 48000.f / 8192.f;

		for(auto i = 0; i < 6; ++i) {
			auto const target = strings[i].getFrequency(chord.getFrets()[i]);
			EXPECT_THAT(frequencies, Contains(FloatNear(target, binWidth)));
			if(strings[i].hasInverseFrequency(chord.getFrets()[i])) {
				auto const inverseTarget = strings[i].getFrequency(chord.getFrets()[i], true);
				EXPECT_THAT(frequencies, Contains(FloatNear(inverseTarget, binWidth)));
			}
		}
	}


	FFT fft{FFT(48000, 8192)};
	std::vector<float> input{std::vector<float>(8192)};
};

TEST_F(UnitTest_FFT, silence) {
	fill(input, std::set<StringPlay>{});

	auto const result = fft.analyze(input);
	auto n = 0;
	for(auto const& item : result) {
		EXPECT_THAT(item.frequency, FloatNear(n * 48000.f / 8192.f, 0.1f));
		EXPECT_THAT(item.power, FloatNear(0.f, 0.1f));
		++n;
	}
}

TEST_F(UnitTest_FFT, a) {
	fill(input, std::set<StringPlay>{StringPlay{strings[1], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[1], 0}});
}

TEST_F(UnitTest_FFT, a_2) {
	fill(input, {StringPlay{strings[1], 12}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[1], 12}});
}

TEST_F(UnitTest_FFT, a_3) {
	fill(input, {StringPlay{strings[1], 24}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[1], 24}});
}

TEST_F(UnitTest_FFT, E) {
	fill(input, {StringPlay{strings[0], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[0], 0}});
}

TEST_F(UnitTest_FFT, d) {
	fill(input, {StringPlay{strings[2], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[2], 0}});
}

TEST_F(UnitTest_FFT, g) {
	fill(input, {StringPlay{strings[3], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[3], 0}});
}

TEST_F(UnitTest_FFT, b) {
	fill(input, {StringPlay{strings[4], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[4], 0}});
}

TEST_F(UnitTest_FFT, a_E) {
	fill(input, {StringPlay{strings[0], 0}, StringPlay{strings[1], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[0], 0}, StringPlay{strings[1], 0}});
}

TEST_F(UnitTest_FFT, a_b) {
	fill(input, {StringPlay{strings[1], 0}, StringPlay{strings[4], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[1], 0}, StringPlay{strings[4], 0}});
}

TEST_F(UnitTest_FFT, a_E_b) {
	fill(input, {StringPlay{strings[0], 0}, StringPlay{strings[1], 0}, StringPlay{strings[4], 0}});

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, {StringPlay{strings[0], 0}, StringPlay{strings[1], 0}, StringPlay{strings[4], 0}});
}

TEST_F(UnitTest_FFT, chord_Em) {
	fill(input, Em);

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, Em);
}

TEST_F(UnitTest_FFT, chord_D) {
	fill(input, D);

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, D);
}

TEST_F(UnitTest_FFT, chord_Dm) {
	fill(input, Dm);

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, Dm);
}

TEST_F(UnitTest_FFT, chord_A) {
	fill(input, A);

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, A);
}

TEST_F(UnitTest_FFT, chord_Am) {
	fill(input, Am);

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, Am);
}

TEST_F(UnitTest_FFT, chord_C) {
	fill(input, C);

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, C);
}

TEST_F(UnitTest_FFT, chord_G) {
	fill(input, G);

	auto const result = fft.analyze(input);

	printBest(result);
	check(result, G);
}


