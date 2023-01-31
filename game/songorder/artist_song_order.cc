#include "artist_song_order.hh"

std::string ArtistSongOrder::getDescription() const {
	return _("sorted by artist");
}

bool ArtistSongOrder::operator()(Song const& a, Song const& b) const {
	return a.collateByArtist < b.collateByArtist;
}

