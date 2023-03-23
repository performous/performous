#pragma once

#include <chrono>
#include <string>

/// This struct holds together information for a single item of a highscore.
struct HiscoreItem {
	unsigned score, playerid, songid;
	unsigned short level;
	std::string track;
	std::chrono::seconds unixtime;

	HiscoreItem(unsigned score, unsigned playerid, unsigned songid, unsigned short level, std::string const& track, std::chrono::seconds unixtime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()))
	: score(score), playerid(playerid), songid(songid), level(level), track(track), unixtime(unixtime) {
	}

	/// Operator for sorting by score. Reverse order, so that highest is first!
	bool operator<(HiscoreItem const& other) const {
		return other.score < score;
	}
};
