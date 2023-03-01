#pragma once

#include "songorder.hh"

struct NameSongOrder : public SongOrder {
	std::string getDescription() const override;

	void prepare(SongCollection const&, Database const&) override;

	bool operator()(Song const& a, Song const& b) const override;
};

