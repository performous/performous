#include "has_error_song_order.hh"

#include "configuration.hh"
#include "unicode.hh"

std::string HasErrorSongOrder::getDescription() const {
	return _("sorted by has_error");
}

void HasErrorSongOrder::prepare(SongCollection const&, Database const&) {
	UnicodeUtil::m_sortCollator->setStrength(config["game/case-sorting"].b() ? icu::Collator::TERTIARY : icu::Collator::SECONDARY);
}

bool HasErrorSongOrder::operator()(Song const& a, Song const& b) const {
	return a.loadStatus < b.loadStatus;
}
