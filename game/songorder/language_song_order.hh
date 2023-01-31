#pragma once

#include "songorder.hh"

struct LanguageSongOrder : public SongOrder {
	std::string getDescription() const override;
	bool operator()(Song const& a, Song const& b) const override;
};

