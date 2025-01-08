#pragma once

#include "fs.hh"

#include <iostream>
#include <stdexcept>
#include <string>

class Song;

/// Thrown by SongParser when there is an error
struct SongParserException: public std::runtime_error {
	SongParserException(Song& s, std::string const& msg, unsigned int linenum, bool sil = false);
	~SongParserException() noexcept = default;

	fs::path const& file() const { return m_filename; } ///< file in which the error occured
	unsigned int line() const { return m_linenum; } ///< line in which the error occured
	bool silent() const { return m_silent; } ///< if the error should not be printed to user (file skipped)

private:
	fs::path m_filename;
	unsigned int m_linenum;
	bool m_silent;
};

/// Print a SongParserException in a format suitable for the logging system.
std::ostream& operator<<(std::ostream& os, SongParserException const& e);

