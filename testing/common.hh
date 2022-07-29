#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <vector>

std::vector<float> loadRaw(std::string const& filepath);

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::IsEmpty;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::Le;
using ::testing::Lt;
using ::testing::Ne;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

