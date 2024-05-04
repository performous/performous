#pragma once

#include <chrono>
#include <string>
#include "songitems.hh"

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

struct HiscoreItemBySongAndLevelKey {
	SongId songid;
	unsigned short level;

	bool operator==(const HiscoreItemBySongAndLevelKey& other) const {
		return songid == other.songid && level == other.level;
	}

	bool operator!=(const HiscoreItemBySongAndLevelKey& other) const {
		return songid != other.songid || level != other.level;
	}

	bool operator<(const HiscoreItemBySongAndLevelKey& other) const {
		if (songid != other.songid)
			return songid < other.songid;
		return level < other.level;
	}
};

template <>
struct std::hash<HiscoreItemBySongAndLevelKey>
{
	std::size_t operator()(const HiscoreItemBySongAndLevelKey& k) const
	{
		using std::size_t;
		using std::hash;
		using std::string;

		// Compute individual hash values for first,
		// and second and combine them using XOR
		// and bit shifting:
		return (hash<SongId>()(k.songid)
			^ (hash<SongId>()(k.level) << 1));
	}
};
