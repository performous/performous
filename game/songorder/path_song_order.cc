#include "path_song_order.hh"

std::string PathSongOrder::getDescription() const {
	return _("sorted by path");
}

bool PathSongOrder::operator()(Song const& a, Song const& b) const {
	return a.path < b.path;
}

