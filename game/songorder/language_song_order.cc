#include "language_song_order.hh"

std::string LanguageSongOrder::getDescription() const {
	return _("sorted by language");
}

bool LanguageSongOrder::operator()(Song const& a, Song const& b) const {
	return a.language < b.language;
}

