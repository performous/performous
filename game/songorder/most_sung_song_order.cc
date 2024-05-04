#include "most_sung_song_order.hh"

#include "database.hh"
#include "profiler.hh"

std::string MostSungSongOrder::getDescription() const {
	return _("sort by most sung");
}

void MostSungSongOrder::initialize(SongCollection const& songs, Database const& database) {
	if (initialized)
		return;

	Profiler prof("MostSungSongOrder_initialize");

	std::for_each(songs.begin(), songs.end(), [&](SongPtr const& song) {
		try {
			m_rateMap[song.get()] = database.getAllHiscoresCount(song);
		}
		catch (std::exception const&) {
			m_rateMap[song.get()] = 0;
		}
	});

	initialized = true;
	prof("initialized");
}

bool MostSungSongOrder::operator()(const Song& a, const Song& b) const {
	if (!initialized)
		return false;

	auto const rateA = m_rateMap.find(&a)->second;
	auto const rateB = m_rateMap.find(&b)->second;

	return rateA > rateB;
}