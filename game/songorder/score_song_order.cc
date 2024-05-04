#include "score_song_order.hh"

#include "database.hh"

std::string ScoreSongOrder::getDescription() const {
	return _("sorted by score");
}

void ScoreSongOrder::prepare(SongCollection const& songs, Database const& database) {
	auto begin = songs.begin();
	auto end = songs.end();

	std::for_each(begin, end, [&](SongPtr const& song) {
		try {
			m_scoreMap[song.get()] = database.getHiscore(song);
		}
		catch(std::exception const&) {
			m_scoreMap[song.get()] = 0;
		}
	});
}

bool ScoreSongOrder::operator()(Song const& a, Song const& b) const {
	auto const scoreA = m_scoreMap.find(&a)->second;
	auto const scoreB = m_scoreMap.find(&b)->second;

	return scoreA > scoreB;
}
