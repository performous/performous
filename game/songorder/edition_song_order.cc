#include "edition_song_order.hh"

#include "configuration.hh"
#include "unicode.hh"

std::string EditionSongOrder::getDescription() const {
	return _("sorted by edition");
}

void EditionSongOrder::prepare(SongCollection const&, Database const&) {
	UnicodeUtil::m_sortCollator->setStrength(config["game/case-sorting"].b() ? icu::Collator::TERTIARY : icu::Collator::SECONDARY);
}

bool EditionSongOrder::operator()(Song const& a, Song const& b) const {
	return a.edition < b.edition;
}


