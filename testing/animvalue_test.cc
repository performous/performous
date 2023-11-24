#include "game/animvalue.hh"

#include "common.hh"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST(UnitTest_AnimValue, default_ctor) {
	EXPECT_NEAR(0.0, AnimValue().get(), 0.000001);
	EXPECT_NEAR(0.0, AnimValue().getTarget(), 0.000001);
}

TEST(UnitTest_AnimValue, setTarget) {
	auto sut = AnimValue();

	sut.setTarget(5.);

	EXPECT_NEAR(0.0, sut.get(), 0.000001);
	EXPECT_NEAR(5.0, sut.getTarget(), 0.000001);
}

TEST(UnitTest_AnimValue, setTarget_with_step) {
	auto sut = AnimValue();

	sut.setTarget(5., true);

	EXPECT_NEAR(5.0, sut.get(), 0.000001);
	EXPECT_NEAR(5.0, sut.getTarget(), 0.000001);
}

TEST(UnitTest_AnimValue, get) {
	auto timePoint = Clock::now();
	auto sut = AnimValue(0., 1., timePoint);

	sut.setTarget(1.);

	EXPECT_NEAR(0.0, sut.get(timePoint), 0.000001);
	EXPECT_NEAR(0.1, sut.get(timePoint + 100ms), 0.000001);
	EXPECT_NEAR(0.2, sut.get(timePoint + 200ms), 0.000001);
	EXPECT_NEAR(0.3, sut.get(timePoint + 300ms), 0.000001);
	EXPECT_NEAR(0.4, sut.get(timePoint + 400ms), 0.000001);
	EXPECT_NEAR(0.5, sut.get(timePoint + 500ms), 0.000001);
}

TEST(UnitTest_AnimValue, get_time_exceeds) {
	auto timePoint = Clock::now();
	auto sut = AnimValue(0., 1., timePoint);

	sut.setTarget(1.);

	EXPECT_NEAR(0.0, sut.get(timePoint), 0.000001);
	EXPECT_NEAR(1.0, sut.get(timePoint + 1200ms), 0.000001);
}

TEST(PerformanceTest_AnimValue, get_1000000) {
	auto sut = AnimValue(0., 1.);
	auto const start = Clock::now();

	for (auto i = 0; i < 1000000; ++i) {
		if(sut.get() < 0.)
			FAIL();
	}

	auto const end = Clock::now();
	auto const duration = end - start;
	auto const microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);

	std::cout << "1000000 get duration " << (double(microseconds.count()) / 1000000.0) << "s" << std::endl;

	EXPECT_LE(microseconds.count(), 2000000); // should be fast enough
}
