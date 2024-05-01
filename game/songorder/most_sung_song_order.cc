#include "most_sung_song_order.hh"

#include "database.hh"

std::string MostSungSongOrder::getDescription() const {
	return _("sort by most sung");
}

bool MostSungSongOrder::operator()(const Song& a, const Song& b) const {
	return a.timesPlayed > b.timesPlayed;
}
