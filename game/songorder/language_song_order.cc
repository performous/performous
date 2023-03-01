#include "language_song_order.hh"

#include "configuration.hh"
#include "unicode.hh"

std::string LanguageSongOrder::getDescription() const {
	return _("sorted by language");
}

void LanguageSongOrder::prepare(SongCollection const&, Database const&) {
	UnicodeUtil::m_sortCollator->setStrength(config["game/case-sorting"].b() ? icu::Collator::TERTIARY : icu::Collator::SECONDARY);
}

bool LanguageSongOrder::operator()(Song const& a, Song const& b) const {
	return a.language < b.language;
}

