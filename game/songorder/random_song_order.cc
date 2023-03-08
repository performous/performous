#include "random_song_order.hh"

#include "i18n.hh"

std::string RandomSongOrder::getDescription() const {
	return _("random order");
}

bool RandomSongOrder::operator()(Song const& a, Song const& b) const {
	return a.randomIdx < b.randomIdx;
}

