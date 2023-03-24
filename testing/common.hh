#pragma once

#include <cmath>

#ifdef WIN32
static constexpr float pi = 3.14159265359f;
static constexpr float pi2 = pi * 2.f;
#else
static const float pi = std::acos(-1.f);
static const float pi2 = pi * 2.f;
#endif

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::IsEmpty;
using ::testing::IsNull;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Not;
using ::testing::NotNull;
using ::testing::Pointee;

