#include "score_song_order.hh"

#include "database.hh"
#include "profiler.hh"

std::string ScoreSongOrder::getDescription() const {
	return _("sorted by score");
}

void ScoreSongOrder::initialize(SongCollection const& songs, Database const& database) {
	if (initialized)
		return;

	Profiler prof("ScoreSongOrder_initialize");

	std::for_each(songs.begin(), songs.end(), [&](SongPtr const& song) {
		try {
			m_scoreMap[song.get()] = database.getHiscore(song);
		}
		catch(std::exception const&) {
			m_scoreMap[song.get()] = 0;
		}
	});

	initialized = true;
	prof("initialized");
}

bool ScoreSongOrder::operator()(Song const& a, Song const& b) const {
	if (!initialized)
		return false;

	auto const scoreA = m_scoreMap.find(&a)->second;
	auto const scoreB = m_scoreMap.find(&b)->second;

	return scoreA > scoreB;
}
