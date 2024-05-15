#pragma once

#include "songorder.hh"

struct MostSungSongOrder : public SongOrder {
	std::string getDescription() const override;

	void initialize(SongCollection const& songs, Database const& database) override;

	void update(SongPtr const& songs, Database const& database) override;

	bool operator()(Song const& a, Song const& b) const override;

private:
	std::unordered_map<Song const*, size_t> m_rateMap;
	bool initialized;
};
