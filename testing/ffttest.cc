#include "common.hh"

#include "game/guitarchords/fft.hh"
#include "game/guitarchords/chord.hh"
#include "game/guitarchords/chords.hh"
#include "game/guitarchords/notes.hh"


struct UnitTest_FFT : public testing::Test {
	float const pi2 = static_cast<float>(M_PI * 2.0);

	void fill(std::vector<float>& data, std::set<float> frequencies) {
		for(auto n = 0; n < data.size(); ++n) {
			auto const nf = static_cast<float>(n);
			auto s = 0.f;
			for(auto frequency : frequencies)
				s += 0.25f * sin(nf * frequency * pi2 / 48000.f);

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
	void check(std::vector<FFTItem> const& result, std::set<float> targets) {
		auto sorted = std::map<float, float>();

		for(auto const& item : result)
			sorted[-item.power] = item.frequency;

		auto best = std::vector<float>(targets.size());
		auto it = sorted.begin();

		for(auto& frequency : best)
			frequency = it++->second;

		auto const binWidth = 48000.f / 8192.f;

		for(auto const& target : targets)
			EXPECT_THAT(best, Contains(FloatNear(target, binWidth)));
	}


	FFT fft{FFT(48000, 8192)};
	std::vector<float> input{std::vector<float>(8192)};
};

TEST_F(UnitTest_FFT, silence) {
	fill(input, {});

	auto const result = fft.analyse(input);
	auto n = 0;
	for(auto const& item : result) {
		EXPECT_THAT(item.frequency, FloatNear(n * 48000.f / 8192.f, 0.1f));
		EXPECT_THAT(item.power, FloatNear(0.f, 0.1f));
		++n;
	}
}

TEST_F(UnitTest_FFT, a) {
	fill(input, {a});

	auto const result = fft.analyse(input);
	auto n = 0;
	for(auto const& item : result) {
		EXPECT_THAT(item.frequency, FloatNear(n * 48000.f / 8192.f, 0.1f));
		if(fabs(item.frequency - a) < 12)
			EXPECT_THAT(-item.power, Lt(-10.f));
		else
			EXPECT_THAT(item.power, FloatNear(0.f, 9.f));
		++n;
	}

	printBest(result);
	check(result, {a});
}

TEST_F(UnitTest_FFT, a_2) {
	fill(input, {a * 2});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {a * 2});
}

TEST_F(UnitTest_FFT, a_3) {
	fill(input, {a * 4});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {a * 4});
}

TEST_F(UnitTest_FFT, E) {
	fill(input, {E});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {E});
}

TEST_F(UnitTest_FFT, d) {
	fill(input, {d});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {d});
}

TEST_F(UnitTest_FFT, g) {
	fill(input, {g});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {g});
}

TEST_F(UnitTest_FFT, b) {
	fill(input, {b});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {b});
}

TEST_F(UnitTest_FFT, a_E) {
	fill(input, {a, E});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {a, E});
}

TEST_F(UnitTest_FFT, a_b) {
	fill(input, {a, b});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {a, b});
}

TEST_F(UnitTest_FFT, a_E_b) {
	fill(input, {a, E, b});

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, {a, E, b});
}

TEST_F(UnitTest_FFT, chord_Em) {
	fill(input, Em);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, Em);
}

TEST_F(UnitTest_FFT, chord_D) {
	fill(input, D);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, D);
}

TEST_F(UnitTest_FFT, chord_Dm) {
	fill(input, Dm);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, Dm);
}

TEST_F(UnitTest_FFT, chord_A) {
	fill(input, A);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, A);
}

TEST_F(UnitTest_FFT, chord_Am) {
	fill(input, Am);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, Am);
}

TEST_F(UnitTest_FFT, chord_C) {
	fill(input, C);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, C);
}

TEST_F(UnitTest_FFT, chord_G) {
	fill(input, G);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, G);
}


