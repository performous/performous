#pragma once

#include "songorder.hh"

struct ScoreSongOrder : public SongOrder {
	std::string getDescription() const override;

	void initialize(SongCollection const& songs, Database const& database) override;

	bool operator()(Song const& a, Song const& b) const override ;

  private:
	std::unordered_map<Song const*, unsigned> m_scoreMap;
	bool initialized;
};
