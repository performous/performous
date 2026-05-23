#pragma once

#include <iostream>

enum class FilterType {
	None = 0,
	HasDance = 1,
	HasVocals = 2,
	HasDuet = 3,
	HasGuitar = 4,
	HasDrumsOrKeyboard = 5,
	FullBand = 6
};

int toInt(FilterType type);

FilterType toFilterType(int type);

std::string toString(FilterType const& type);
std::ostream& operator<<(std::ostream&, FilterType const&);
