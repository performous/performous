#include "name_song_order.hh"

std::string NameSongOrder::getDescription() const {
	return _("sorted by song");
}

bool NameSongOrder::operator()(Song const& a, Song const& b) const {
	return a.collateByTitle < b.collateByTitle;
}
