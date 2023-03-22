#pragma once

class Song;

struct ISongParser {
	virtual ~ISongParser() = default;

	virtual void parse(Song&) = 0;
};


