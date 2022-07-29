#include "common.hh"

#include "game/guitarchords/fft.hh"
#include "game/guitarchords/chord.hh"
#include "game/guitarchords/chords.hh"
#include "game/guitarchords/notes.hh"

#include <fstream>

struct UnitTest_FFT_sample : public testing::Test {
	std::vector<String> const strings{E0, a0, d0, g0, b0, e0};

	void fill(std::string const& filepath, size_t offset = 0) {
		if(filepath.substr(filepath.size() - 4) == ".f32") {
			auto const buffer = loadRaw(filepath);

			for(auto i = 0; i < input.size() && i + offset < buffer.size(); ++i)
				input[i] = buffer[i + offset];

			return;
		}

		throw std::logic_error("File other than raw are not supported yet!");
	}

	void check(std::vector<FFTItem> const& result, std::set<StringPlay> const& targets) {
		auto frequencies = std::set<float>();

		for(auto const& item : result)
			if(item.power > 0.1)
				frequencies.insert(item.frequency);

		auto const binWidth = 48000.f / 8192.f * 1.5f;

		for(auto const& target : targets) {
			EXPECT_THAT(frequencies, Contains(FloatNear(target.getFrequency(), binWidth)));
			if(target.hasInverseFrequency())
				EXPECT_THAT(frequencies, Contains(FloatNear(target.getFrequency(true), binWidth)));
		}
	}
	void check(std::vector<FFTItem> const& result, std::vector<Fret> const& frets) {
		auto frequencies = std::set<float>();

		for(auto const& item : result)
			if(item.power > 0.1)
				frequencies.insert(item.frequency);

		auto const binWidth = 48000.f / 8192.f * 1.5f;

		for(auto i = 0; i < 6; ++i) {
			auto const target = strings[i].getFrequency(frets[i]);
			EXPECT_THAT(frequencies, Contains(FloatNear(target, binWidth)));
			if(strings[i].hasInverseFrequency(frets[i])) {
				auto const inverseTarget = strings[i].getFrequency(frets[i], true);
				EXPECT_THAT(frequencies, Contains(FloatNear(inverseTarget, binWidth)));
			}
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

	void printSample(std::vector<float> const& samples) {
		for(auto const& v : samples)
			std::cout << v << "/";

		std::cout << std::endl;
	}

	FFT fft{FFT(48000, 8192)};
	std::vector<float> input{std::vector<float>(8192)};
};


TEST_F(UnitTest_FFT_sample, E) {
	fill("data/guitar/takamine/E.f32", 4800);

	//printSample(input);

	auto const result = fft.analyze(input);

	check(result, {StringPlay{strings[0], 0}});
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, a) {
	fill("data/guitar/takamine/a.f32");

	auto const result = fft.analyze(input);

	check(result, {StringPlay{strings[1], 0}});
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, d) {
	fill("data/guitar/takamine/d.f32");

	auto const result = fft.analyze(input);

	check(result, {StringPlay{strings[2], 0}});
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, g) {
	fill("data/guitar/takamine/g.f32");

	auto const result = fft.analyze(input);

	check(result, {StringPlay{strings[3], 0}});
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, b) {
	fill("data/guitar/takamine/b.f32");

	auto const result = fft.analyze(input);

	check(result, {StringPlay{strings[4], 0}});
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, e) {
	fill("data/guitar/takamine/e.f32");

	auto const result = fft.analyze(input);

	check(result, {StringPlay{strings[5], 0}});
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, DISABLED_chord_D) {
	fill("data/guitar/takamine/D.f32", 4800);

	auto const result = fft.analyze(input);

	check(result, D);
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, DISABLED_chord_Dm) {
	fill("data/guitar/takamine/Dm.f32", 4800);

	auto const result = fft.analyze(input);

	check(result, Dm);
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, chord_G) {
	fill("data/guitar/takamine/G-2.f32", 4800);

	auto const result = fft.analyze(input);

	check(result, G);
//	printBest(result);
}

TEST_F(UnitTest_FFT_sample, chord_A) {
	fill("data/guitar/takamine/A.f32", 4800);

	auto const result = fft.analyze(input);

	check(result, A);
	printBest(result);
}

TEST_F(UnitTest_FFT_sample, chord_Em) {
	fill("data/guitar/takamine/Em.f32", 4800);

	auto const result = fft.analyze(input);

	check(result, Em);
	printBest(result);
}
