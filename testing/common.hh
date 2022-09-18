#pragma once

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif

#include <cmath>

#ifdef WIN32
static constexpr float pi = M_PI;
static constexpr float pi2 = pi * 2.f;
#else
static constexpr float pi = std::acos(-1.f);
static constexpr float pi2 = pi * 2.f;
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

