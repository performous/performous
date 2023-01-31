#pragma once

#include "songorder.hh"

struct GenreSongOrder : public SongOrder {
	std::string getDescription() const override;
	bool operator()(Song const& a, Song const& b) const override;
};

