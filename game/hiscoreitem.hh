#pragma once

#include <string>

/// This struct holds together information for a single item of a highscore.
struct HiscoreItem {
	unsigned score, playerid, songid, level;
	std::string track;
	HiscoreItem(unsigned score, unsigned playerid, unsigned songid, unsigned level, std::string const& track):
	  score(score), playerid(playerid), songid(songid), level(level), track(track) {}
	/// Operator for sorting by score. Reverse order, so that highest is first!
	bool operator<(HiscoreItem const& other) const { return other.score < score; }
};
