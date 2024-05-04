#pragma once

#include "songorder.hh"

#include <chrono>

struct FileTimeSongOrder : public SongOrder {
	std::string getDescription() const override;

	void initialize(SongCollection const& , Database const&) override;

	bool operator()(Song const& a, Song const& b) const override;

  private:
	std::unordered_map<Song const*, std::chrono::seconds> m_dateMap;
	bool initialized;
};
