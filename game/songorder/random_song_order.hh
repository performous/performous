#pragma once

#include "songorder.hh"

struct RandomSongOrder : public SongOrder {
	std::string getDescription() const override;
	bool operator()(Song const& a, Song const& b) const override;
};

