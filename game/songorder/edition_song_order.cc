#include "edition_song_order.hh"

std::string EditionSongOrder::getDescription() const {
	return _("sorted by edition");
}

bool EditionSongOrder::operator()(Song const& a, Song const& b) const {
	return a.edition < b.edition;
}


