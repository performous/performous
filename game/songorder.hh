#pragma once

#include "song.hh"

#include <memory>
#include <string>

class Database;

struct SongOrder {
	virtual ~SongOrder() = default;

	virtual std::string getDescription() const = 0;
	virtual void prepare(SongCollection const&, Database const&) {}

	virtual bool operator()(Song const& a, Song const& b) const = 0;
};

using SongOrderPtr = std::shared_ptr<SongOrder>;
