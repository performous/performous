#pragma once

#include "songorder.hh"

struct NameSongOrder : public SongOrder {
	std::string getDescription() const override;
	bool operator()(Song const& a, Song const& b) const override;
};

