#include "name_song_order.hh"

#include "configuration.hh"
#include "unicode.hh"

std::string NameSongOrder::getDescription() const {
	return _("sorted by song");
}

void NameSongOrder::prepare(SongCollection const&, Database const&) {
	UnicodeUtil::m_sortCollator->setStrength(config["game/case-sorting"].b() ? icu::Collator::TERTIARY : icu::Collator::SECONDARY);
}

bool NameSongOrder::operator()(Song const& a, Song const& b) const {
	return a.collateByTitle < b.collateByTitle;
}
