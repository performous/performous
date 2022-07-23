#include "common.hh"

#include "game/ringbuffer.hh"

#include <vector>

TEST(UnitTest_RingBuffer, default_ctor) {
	EXPECT_EQ(0, RingBuffer<16>().size());
	EXPECT_EQ(16, RingBuffer<16>().capacity);
}

TEST(UnitTest_RingBuffer, insert) {
	auto buffer = RingBuffer<16>();

	EXPECT_EQ(0, buffer.size());

	auto const data = std::vector<float>{1.f, 2.f};

	buffer.insert(data.begin(), data.end());

	EXPECT_EQ(2, buffer.size());
}

TEST(UnitTest_RingBuffer, insert_2) {
	auto buffer = RingBuffer<16>();

	EXPECT_EQ(0, buffer.size());

	auto const data = std::vector<float>{1.f, 2.f, 3.f};

	buffer.insert(data.begin(), data.end());
	buffer.insert(data.begin(), data.end());

	EXPECT_EQ(6, buffer.size());
}

TEST(UnitTest_RingBuffer, insert_fill) {
	auto buffer = RingBuffer<4>();

	EXPECT_EQ(0, buffer.size());

	auto const data = std::vector<float>{1.f, 2.f, 3.f, 4.f};

	buffer.insert(data.begin(), data.end());

	EXPECT_EQ(4, buffer.size());
}

TEST(UnitTest_RingBuffer, insert_1_overflow) {
	auto buffer = RingBuffer<4>();

	EXPECT_EQ(0, buffer.size());

	auto const data = std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f};

	buffer.insert(data.begin(), data.end());

	EXPECT_EQ(4, buffer.size());

	auto dataOut = std::vector<float>{0.f,0.f,0.f,0.f};

	buffer.read(dataOut.begin(), dataOut.end());

	EXPECT_EQ(4, buffer.size());
	EXPECT_THAT(dataOut, ElementsAre(3.f, 4.f, 5.f, 6.f));
}

TEST(UnitTest_RingBuffer, insert_2_overflow) {
	auto buffer = RingBuffer<4>();

	EXPECT_EQ(0, buffer.size());

	auto const data0 = std::vector<float>{1.f, 2.f, 3.f};
	auto const data1 = std::vector<float>{4.f, 5.f, 6.f};

	buffer.insert(data0.begin(), data0.end());
	buffer.insert(data1.begin(), data1.end());

	EXPECT_EQ(4, buffer.size());

	auto dataOut = std::vector<float>(4);

	buffer.read(dataOut.begin(), dataOut.end());

	EXPECT_EQ(4, buffer.size());
	EXPECT_THAT(dataOut, ElementsAre(3.f, 4.f, 5.f, 6.f));
}

TEST(UnitTest_RingBuffer, read) {
	auto buffer = RingBuffer<16>();

	EXPECT_EQ(0, buffer.size());

	auto const dataIn = std::vector<float>{1.f, 2.f, 3.f};

	buffer.insert(dataIn.begin(), dataIn.end());

	auto dataOut = std::vector<float>{0.f, 0.f};

	EXPECT_TRUE(buffer.read(dataOut.begin(), dataOut.end()));
	EXPECT_EQ(3, buffer.size());
	EXPECT_THAT(dataOut, ElementsAre(1.f, 2.f));
}

TEST(UnitTest_RingBuffer, read_to_empty) {
	auto buffer = RingBuffer<16>();

	EXPECT_EQ(0, buffer.size());

	auto const dataIn = std::vector<float>{1.f, 2.f, 3.f};

	buffer.insert(dataIn.begin(), dataIn.end());

	auto dataOut = std::vector<float>{};

	EXPECT_TRUE(buffer.read(dataOut.begin(), dataOut.end()));
	EXPECT_EQ(3, buffer.size());
	EXPECT_THAT(dataOut, ElementsAre());
}

TEST(UnitTest_RingBuffer, read_underflow) {
	auto buffer = RingBuffer<16>();

	EXPECT_EQ(0, buffer.size());

	auto const dataIn = std::vector<float>{1.f, 2.f, 3.f};

	buffer.insert(dataIn.begin(), dataIn.end());

	auto dataOut = std::vector<float>(4);

	EXPECT_FALSE(buffer.read(dataOut.begin(), dataOut.end()));
	EXPECT_EQ(3, buffer.size());
	EXPECT_THAT(dataOut, ElementsAre(0.f, 0.f, 0.f, 0.f));
}

