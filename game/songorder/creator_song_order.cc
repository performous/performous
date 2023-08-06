#include "creator_song_order.hh"

#include "configuration.hh"
#include "unicode.hh"

std::string CreatorSongOrder::getDescription() const {
	return _("sorted by creator");
}

void CreatorSongOrder::prepare(SongCollection const&, Database const&) {
	UnicodeUtil::m_sortCollator->setStrength(config["game/case-sorting"].b() ? icu::Collator::TERTIARY : icu::Collator::SECONDARY);
}

bool CreatorSongOrder::operator()(Song const& a, Song const& b) const {
	return a.creator < b.creator;
}


