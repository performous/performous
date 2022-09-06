#include "common.hh"

#include "game/guitarchords/chorddetector.hh"
#include "game/guitarchords/chords.hh"

namespace {
	struct SilenceSignal : public ISignal {
		~SilenceSignal() override = default;

		void readTo(std::vector<float>& buffer) override {
			for(auto& value : buffer)
				value = 0.f;
		}
	};
	struct SampleSignal : public ISignal {
		SampleSignal(std::vector<float> const& buffer, size_t offset = 0)
		: m_buffer(buffer), m_offset(offset) {
		}
		~SampleSignal() override = default;

		void readTo(std::vector<float>& buffer) override {
			for(auto i = 0U; i < buffer.size(); ++i)
				buffer[i] = m_buffer[i + m_offset];
		}

	private:
		std::vector<float> m_buffer;
		size_t m_offset;
	};
	Profile makeUnity() {
		auto magnitudeFactors = Profile::Factors();
		auto inverseMagnitudeFactors = Profile::Factors();

		for(auto i = 0; i < 8; ++i) {
			magnitudeFactors.emplace_back(std::array<float, 6>{1.f, 1.f, 1.f, 1.f, 1.f, 1.f});
			inverseMagnitudeFactors.emplace_back(std::array<float, 6>{1.f, 1.f, 1.f, 1.f, 1.f, 1.f});
		}

		return Profile("unity", magnitudeFactors, inverseMagnitudeFactors);
	}
}

TEST(UnitTest_ChordDetector, silence_G) {
	auto const detector = ChordDetector(makeUnity());
	auto signal = SilenceSignal();

	EXPECT_EQ(0, detector.detect(G, signal));
}

TEST(UnitTest_ChordDetector, silence_D) {
	auto const detector = ChordDetector(makeUnity());
	auto signal = SilenceSignal();

	EXPECT_EQ(0, detector.detect(D, signal));
}

TEST(UnitTest_ChordDetector, silence_Dm) {
	auto const detector = ChordDetector(makeUnity());
	auto signal = SilenceSignal();

	EXPECT_EQ(0, detector.detect(Dm, signal));
}

TEST(UnitTest_ChordDetector, G_G) {
	auto const detector = ChordDetector(makeUnity());
	auto count = 0;

	for(auto i = 2; i < 12; ++i) {
		auto signal = SampleSignal(loadRaw("data/guitar/takamine/G-2.f32"), 4800 * i);

		if(detector.detect(G, signal))
			++count;
	}

	EXPECT_THAT(count, Ge(8));
}

TEST(UnitTest_ChordDetector, E_E) {
	auto const detector = ChordDetector(makeUnity());
	auto count = 0;

	for(auto i = 2; i < 12; ++i) {
		auto signal = SampleSignal(loadRaw("data/guitar/takamine/E.f32"), 4800 * i);

		if(detector.detect(E, signal))
			++count;
	}

	EXPECT_THAT(count, Ge(8));
}

TEST(UnitTest_ChordDetector, Em_Em) {
	auto const detector = ChordDetector(makeUnity());
	auto count = 0;

	for(auto i = 2; i < 12; ++i) {
		auto signal = SampleSignal(loadRaw("data/guitar/takamine/Em.f32"), 4800 * i);

		if(detector.detect(Em, signal))
			++count;
	}

	EXPECT_THAT(count, Ge(8));
}


TEST(UnitTest_ChordDetector, E_Em) {
	auto const detector = ChordDetector(makeUnity());
	auto count = 0;

	for(auto i = 2; i < 12; ++i) {
		auto signal = SampleSignal(loadRaw("data/guitar/takamine/E.f32"), 4800 * i);

		if(detector.detect(Em, signal))
			++count;
	}

	EXPECT_THAT(count, Eq(0));
}
