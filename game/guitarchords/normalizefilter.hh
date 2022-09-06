#pragma once

#include <cmath>
#include <vector>

struct Normalize  {
	Normalize(std::vector<float> const& buffer)
	: buffer(buffer) {
	}

	float operator()(float v) {
		auto peak = 0.f;

		for(auto i = 0; i < 480; ++i)
			peak = std::max<float>(peak, std::fabs(buffer[i + n]));

		++n;

		if(peak > 0)
			return v / peak;

		return v;
	}

	std::vector<float> const& buffer;
	std::size_t n = 0;
};
