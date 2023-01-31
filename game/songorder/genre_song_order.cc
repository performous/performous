#include "genre_song_order.hh"

std::string GenreSongOrder::getDescription() const {
	return _("sorted by genre");
}

bool GenreSongOrder::operator()(Song const& a, Song const& b) const {
	return a.genre < b.genre;
}

