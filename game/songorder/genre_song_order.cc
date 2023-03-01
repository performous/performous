#include "genre_song_order.hh"

#include "configuration.hh"
#include "unicode.hh"

std::string GenreSongOrder::getDescription() const {
	return _("sorted by genre");
}

void GenreSongOrder::prepare(SongCollection const&, Database const&) {
	UnicodeUtil::m_sortCollator->setStrength(config["game/case-sorting"].b() ? icu::Collator::TERTIARY : icu::Collator::SECONDARY);
}

bool GenreSongOrder::operator()(Song const& a, Song const& b) const {
	return a.genre < b.genre;
}

