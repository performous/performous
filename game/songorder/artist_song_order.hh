#pragma once

#include "songorder.hh"

struct ArtistSongOrder : public SongOrder {
	std::string getDescription() const override;

	void prepare(SongCollection const&, Database const&) override;

	bool operator()(Song const& a, Song const& b) const override;
};
