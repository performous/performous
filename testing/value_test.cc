#include "common.hh"

#include "game/value/value.hh"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST(UnitTest_Value, float_initialization) {
	auto const sut = Value(0.0f);

	EXPECT_NEAR(sut.get(), 0.f, 0.000000001);
}

TEST(UnitTest_Value, not_same_after_copy) {
	auto const sut = Value(0.0f);
	auto sut2 = sut;

	sut2 = Value(1.f);

	EXPECT_THAT(sut2.get(), Ne(sut.get()));
}

TEST(UnitTest_Value, restart_time_on_copy) {
	auto const sut = Value(value::Time());

	while (sut.get() == 0.f)
		std::this_thread::sleep_for(1ms);

	auto const sut2 = sut;

	EXPECT_NEAR(sut2.get(), 0.f, 0.000001);
	EXPECT_THAT(sut2.get(), Ne(sut.get()));
}
