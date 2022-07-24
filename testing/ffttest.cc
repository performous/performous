#include "common.hh"

#include "game/guitarchords/fft.hh"

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
	void fill(std::vector<float>& data) {
		fill(data, std::set<float>{});
	}
	void fill(std::vector<float>& data, float frequency) {
		fill(data, std::set<float>{frequency});
	}
	void fill(std::vector<float>& data, float frequency0, float frequency1) {
		fill(data, std::set<float>{frequency0, frequency1});
	}
	void fill(std::vector<float>& data, float frequency0, float frequency1, float frequency2) {
		fill(data, std::set<float>{frequency0, frequency1, frequency2});
	}
	void fill(std::vector<float>& data, float frequency0, float frequency1, float frequency2, float frequency3) {
		fill(data, std::set<float>{frequency0, frequency1, frequency2, frequency3});
	}
	void fill(std::vector<float>& data, float frequency0, float frequency1, float frequency2, float frequency3, float frequency4) {
		fill(data, std::set<float>{frequency0, frequency1, frequency2, frequency3, frequency4});
	}
	void fill(std::vector<float>& data, float frequency0, float frequency1, float frequency2, float frequency3, float frequency4, float frequency5) {
		fill(data, std::set<float>{frequency0, frequency1, frequency2, frequency3, frequency4, frequency5});
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
	void check(std::vector<FFTItem> const& result, float target) {
		auto sorted = std::map<float, float>();

		for(auto const& item : result)
			sorted[-item.power] = item.frequency;

		EXPECT_THAT(fabs(target - sorted.begin()->second), Le(48000.f / 8192.f));
	}
	void check(std::vector<FFTItem> const& result, float target0, float target1) {
		auto sorted = std::map<float, float>();

		for(auto const& item : result)
			sorted[-item.power] = item.frequency;

		auto best = std::vector<float>(2);
		auto it = sorted.begin();

		for(auto& frequency : best)
			frequency = it++->second;

		auto const binWidth = 48000.f / 8192.f;

		EXPECT_THAT(best, UnorderedElementsAre(FloatNear(target0, binWidth), FloatNear(target1, binWidth)));
	}
	void check(std::vector<FFTItem> const& result, float target0, float target1, float target2) {
		check(result, {target0, target1, target2});
	}
	void check(std::vector<FFTItem> const& result, float target0, float target1, float target2, float target3) {
		check(result, {target0, target1, target2, target3});
	}
	void check(std::vector<FFTItem> const& result, float target0, float target1, float target2, float target3, float target4) {
		check(result, {target0, target1, target2, target3, target4});
	}
	void check(std::vector<FFTItem> const& result, float target0, float target1, float target2, float target3, float target4, float target5) {
		check(result, {target0, target1, target2, target3, target4, target5});
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
	fill(input);

	auto const result = fft.analyse(input);
	auto n = 0;
	for(auto const& item : result) {
		EXPECT_THAT(item.frequency, FloatNear(n * 48000.f / 8192.f, 0.1f));
		EXPECT_THAT(item.power, FloatNear(0.f, 0.1f));
		++n;
	}
}

TEST_F(UnitTest_FFT, A) {
	fill(input, A);

	auto const result = fft.analyse(input);
	auto n = 0;
	for(auto const& item : result) {
		EXPECT_THAT(item.frequency, FloatNear(n * 48000.f / 8192.f, 0.1f));
		if(fabs(item.frequency - A) < 12)
			EXPECT_THAT(-item.power, Lt(-10.f));
		else
			EXPECT_THAT(item.power, FloatNear(0.f, 9.f));
		++n;
	}

	printBest(result);
	check(result, A);
}

TEST_F(UnitTest_FFT, A_2) {
	fill(input, A * 2);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, A * 2);
}

TEST_F(UnitTest_FFT, A_3) {
	fill(input, A * 4);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, A * 4);
}

TEST_F(UnitTest_FFT, E) {
	fill(input, E);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, E);
}

TEST_F(UnitTest_FFT, d) {
	fill(input, d);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, d);
}

TEST_F(UnitTest_FFT, g) {
	fill(input, g);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, g);
}

TEST_F(UnitTest_FFT, b) {
	fill(input, b);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, b);
}

TEST_F(UnitTest_FFT, A_E) {
	fill(input, A, E);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, A, E);
}

TEST_F(UnitTest_FFT, A_b) {
	fill(input, A, b);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, A, b);
}

TEST_F(UnitTest_FFT, A_E_b) {
	fill(input, A, E, b);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, A, E, b);
}

TEST_F(UnitTest_FFT, chord_Em) {
	fill(input, A1, d1, g, b, e);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, A1, d1, g, b, e);
}

TEST_F(UnitTest_FFT, chord_D) {
	fill(input, d, g2, b3, e2);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, d, g2, b3, e2);
}

TEST_F(UnitTest_FFT, chord_Dm) {
	fill(input, d, g2, b3, e1);

	auto const result = fft.analyse(input);

	printBest(result);
	check(result, d, g2, b3, e1);
}


