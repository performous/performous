#pragma once

#include "songorder.hh"

struct ArtistSongOrder : public SongOrder {
	std::string getDescription() const override;
	bool operator()(Song const& a, Song const& b) const override;
};
