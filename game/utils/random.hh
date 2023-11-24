#pragma once

#include <limits>
#include <random>

namespace {
	std::random_device randomDevice;
	std::mt19937 mt(randomDevice());
}

template<class ValueType>
ValueType random(ValueType const& min, ValueType const& max) {
	auto const upperBound = std::numeric_limits<ValueType>::max();
	auto const maxNext = static_cast<ValueType>(std::nextafter(max, upperBound));

	if constexpr (std::is_integral_v<ValueType>) {
		auto&& distribution = std::uniform_int_distribution<ValueType>(min, maxNext);

		return distribution(mt);
	}
	else if (std::is_floating_point_v<ValueType>) {
		auto&& distribution = std::uniform_real_distribution<ValueType>(min, maxNext);

		return distribution(mt);
	}
}

template<class ValueType>
ValueType random(ValueType const& max) {
	return random(static_cast<ValueType>(0), max);
}
