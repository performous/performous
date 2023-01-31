#pragma once

#include "songorder.hh"

struct ScoreSongOrder : public SongOrder {
	std::string getDescription() const override;

	void prepare(SongCollection const& songs, Database const& database) override;

	bool operator()(Song const& a, Song const& b) const override ;

  private:
	std::map<Song const*, unsigned> m_scoreMap;
};

