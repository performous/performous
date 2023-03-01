#include "artist_song_order.hh"

#include "configuration.hh"
#include "unicode.hh"

std::string ArtistSongOrder::getDescription() const {
	return _("sorted by artist");
}

void ArtistSongOrder::prepare(SongCollection const&, Database const&) {
	UnicodeUtil::m_sortCollator->setStrength(config["game/case-sorting"].b() ? icu::Collator::TERTIARY : icu::Collator::SECONDARY);
}

bool ArtistSongOrder::operator()(Song const& a, Song const& b) const {
	return a.collateByArtist < b.collateByArtist;
}

