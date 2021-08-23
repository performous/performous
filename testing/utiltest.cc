#include <gtest/gtest.h>

#include "game/util.hh"

namespace {
    TEST(UnitTest_Utils, clamp) {
        EXPECT_EQ(0.0, clamp(-1.0, 0.0, 1.0));
        EXPECT_EQ(0.5, clamp(0.5, 0.0, 1.0));
        EXPECT_EQ(1.0, clamp(2.0, 0.0, 1.0));
    }
} 
