#include "most_sung_song_order.hh"

#include "database.hh"

std::string MostSungSongOrder::getDescription() const {
	return _("sort by most sung");
}

void MostSungSongOrder::prepare(SongCollection const& songs, Database const& database) {
	auto begin = songs.begin();
	auto end = songs.end();

	std::for_each(begin, end, [&](SongPtr const& song) {
		try {
			m_rateMap[song.get()] = database.getHiscores(song).size();
		}
		catch(std::exception const&) {
			m_rateMap[song.get()] = 0;
		}
	});
}

bool MostSungSongOrder::operator()(const Song& a, const Song& b) const {
	auto const rateA = m_rateMap.find(&a)->second;
	auto const rateB = m_rateMap.find(&b)->second;

	return rateA > rateB;
}



