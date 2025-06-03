#include "../game/log.hh"

#include <gtest/gtest.h>

int main(int argc, char** argv) {
	SpdLogger spdLogger(spdlog::level::critical);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
