#pragma once

#include "isongparser.hh"

#include <memory>

class Song;

/// Determines a song file's format and constructs the matching ISongParser.
class SongParserFactory {
public:
	/// Reads and validates song.filename, sets song.type, and returns a parser for the detected format.
	std::unique_ptr<ISongParser> create(Song& song) const;
};
