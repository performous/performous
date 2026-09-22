#pragma once

#include "color.hh"
#include "controllers.hh"

struct ScoreItem {
	unsigned score;
	input::DevType type;
	std::string track;  ///< includes difficulty
	std::string track_simple; ///< no difficulty
	std::string player_id; ///< identifies the score's source device (microphone name or similar), used to remember player selections
	Color color;

	ScoreItem() = default;
	ScoreItem(unsigned score, input::DevType type, std::string const& track, std::string const& track_simple, Color const& color)
	: score(score), type(type), track(track), track_simple(track_simple), color(color) {}
    
	bool operator<(ScoreItem const& other) const { 
		return score < other.score; 
	}
};
