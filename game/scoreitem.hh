#pragma once

#include "color.hh"
#include "controllers.hh"

struct ScoreItem {
	unsigned score;
	input::DevType type;
	std::string track;  ///< includes difficulty
	std::string track_simple; ///< no difficulty
	Color color;

	ScoreItem() = default;
	ScoreItem(unsigned score, input::DevType type, std::string const& track, std::string const& track_simple, Color const& color)
	: score(score), type(type), track(track), track_simple(track_simple), color(color) {}
    
	bool operator<(ScoreItem const& other) const { 
		return score < other.score; 
	}
};
