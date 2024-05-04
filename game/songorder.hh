#pragma once

#include "song.hh"

#include <memory>
#include <string>

class Database;

struct SongOrder {
	virtual ~SongOrder() = default;

	virtual std::string getDescription() const = 0;

	// called when all songs are loaded or this SongOrder type is selected, ALL songs are passed to this function
	virtual void initialize(SongCollection const&, Database const&) {}

	// called before every sort
	virtual void prepare(SongCollection const&, Database const&) {}

	// called when data related to a song changed
	virtual void update(SongPtr const&, Database const&) {}

	virtual bool operator()(Song const& a, Song const& b) const = 0;
};

using SongOrderPtr = std::shared_ptr<SongOrder>;
