#include "recently_sung_order.hh"
#include "database.hh"

#include <filesystem>

namespace {
	std::chrono::seconds getLastSungTime(SongPtr const& song, Database const& database) {
		auto hiscores = database.getHiscores(song);

		if (hiscores.empty())
			return std::chrono::seconds::zero();

		std::stable_sort(hiscores.begin(), hiscores.end(),
			[&](HiscoreItem const& a, HiscoreItem const& b) { return b.unixtime < a.unixtime; });

		return hiscores.at(0).unixtime;
	}
}

std::string RecentlySungSongOrder::getDescription() const {
	return _("sort by recently sung");
}

void RecentlySungSongOrder::initialize(SongCollection const& songs, Database const& database) {
	if (initialized)
		return;

	std::for_each(songs.begin(), songs.end(), [&](SongPtr const& song) {
		m_dateMap[song.get()] = getLastSungTime(song, database);
	});

	initialized = true;
}

void RecentlySungSongOrder::update(SongPtr const& song, Database const& database) {
	if (!initialized)
		return;

	m_dateMap[song.get()] = getLastSungTime(song, database);
}

bool RecentlySungSongOrder::operator()(const Song& a, const Song& b) const {
	auto const dateA = m_dateMap.find(&a)->second;
	auto const dateB = m_dateMap.find(&b)->second;

	return dateA > dateB;
}

