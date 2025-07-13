#pragma once

#include <memory>
#include <string>

class Song;

struct ISongFilter {
	virtual ~ISongFilter() = default;

	virtual bool filter(Song const&) const = 0;
	virtual std::string toString() const = 0;
};

using SongFilterPtr = std::shared_ptr<ISongFilter>;

