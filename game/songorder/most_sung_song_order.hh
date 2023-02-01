#pragma once

#include "songorder.hh"

struct MostSungSongOrder : public SongOrder {
	std::string getDescription() const override;

	void prepare(SongCollection const& songs, Database const& database) override;

	bool operator()(Song const& a, Song const& b) const override;

  private:
	std::map<Song const*, size_t> m_rateMap;
};

