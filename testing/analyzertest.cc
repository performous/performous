#include "common.hh"
#include "printer.hh"

#include "game/analyzer.hh"

struct UnitTest_Analyzer : public testing::Test {
	float const pi2 = static_cast<float>(M_PI * 2.0);

	float makeWave(float n, float frequency) {
		return sin(n * frequency * pi2 / 48000.f);
	}

	void fill(std::set<float> const& frequencies) {
		auto data = std::vector<float>(8192);

		for(auto n = 0; n < data.size(); ++n) {
			auto const nf = static_cast<float>(n);
			auto s = 0.f;
			auto i = 0;
			for(auto const& frequency : frequencies) {
				s += 0.25f * makeWave(nf, frequency);
			}

			data[n] = s;
		}

		analyzer.input(data.begin(), data.end());
	}

	Analyzer analyzer{Analyzer(48000, "id")};
	std::vector<float> input{std::vector<float>(8192)};
};

TEST_F(UnitTest_Analyzer, getId) {
	EXPECT_THAT(analyzer.getId(), "id");
	EXPECT_THAT(Analyzer(11025, "test").getId(), "test");
}

TEST_F(UnitTest_Analyzer, getTones_silence) {
	fill({});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, ElementsAre());
}

TEST_F(UnitTest_Analyzer, getTones_32_7_C) {
	fill({32.7});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(32.7));
}

TEST_F(UnitTest_Analyzer, getTones_36_7_D) {
	fill({36.7});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(36.7));
}

TEST_F(UnitTest_Analyzer, getTones_41_2_E) {
	fill({41.2});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(41.2));
}

TEST_F(UnitTest_Analyzer, getTones_43_7_F) {
	fill({43.7});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(43.7));
}

TEST_F(UnitTest_Analyzer, getTones_49_0_G) {
	fill({49});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(49));
}

TEST_F(UnitTest_Analyzer, getTones_55_0_A) {
	fill({55});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(55));
}

TEST_F(UnitTest_Analyzer, getTones_110) {
	fill({110});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(110));
}

TEST_F(UnitTest_Analyzer, getTones_220) {
	fill({220});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(220));
}

TEST_F(UnitTest_Analyzer, getTones_440) {
	fill({440});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(440));
}

TEST_F(UnitTest_Analyzer, getTones_880) {
	fill({880});

	analyzer.process();

	auto const& result = analyzer.getTones();

	EXPECT_THAT(result, Contains(880));
}


TEST_F(UnitTest_Analyzer, findTone_silence) {
	fill({});

	analyzer.process();

	auto const result = analyzer.findTone();

	EXPECT_THAT(result, IsNull());
}

TEST_F(UnitTest_Analyzer, findTone_32_7_C) {
	fill({32.7});

	analyzer.process();

	auto const result = analyzer.findTone();

	EXPECT_THAT(result, IsNull()); // 32.7 is outside used ranged
}

TEST_F(UnitTest_Analyzer, findTone_65_4_C) {
	fill({65.4});

	analyzer.process();

	auto const result = analyzer.findTone();

	EXPECT_THAT(result, AllOf(NotNull(), Pointee(65.4)));
}

TEST_F(UnitTest_Analyzer, findTone_73_4_D) {
	fill({73.4});

	analyzer.process();

	auto const& result = analyzer.findTone();

	EXPECT_THAT(result, AllOf(NotNull(), Pointee(73.4)));
}

TEST_F(UnitTest_Analyzer, findTone_82_4_E) {
	fill({82.4});

	analyzer.process();

	auto const& result = analyzer.findTone();

	EXPECT_THAT(result, AllOf(NotNull(), Pointee(82.4)));
}

TEST_F(UnitTest_Analyzer, findTone_1760_A) {
	fill({1760});

	analyzer.process();

	auto const& result = analyzer.findTone();

	EXPECT_THAT(result, IsNull()); // 1760 is outside used ranged
}
