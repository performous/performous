#pragma once

#include "song.hh"

#include <functional>
#include <istream>
#include <string>

namespace SongParserUtil {
	const std::string DUET_P2 = "Duet singer";	// FIXME
	const std::string DUET_BOTH = "Both singers";	// FIXME
	/// Parse an int from string and assign it to a variable
	void assign(int& var, std::string const& str);
	/// Parse an unsigned int from string and assign it to a variable
	void assign(unsigned& var, std::string const& str);
	/// Parse a double from string and assign it to a variable
	void assign(double& var, std::string str);
	/// Parse a float from string and assign it to a variable
	void assign(float& var, std::string str);
	/// Parse a boolean from string and assign it to a variable
	void assign(bool& var, std::string const& str);
	/// Erase last character if it matches
	void eraseLast(std::string& s, char ch = ' ');

	/// Read a line, tracking the line number for error reporting
	bool getLine(std::istream& stream, std::string& line, unsigned& linenum);
	/// Find the BPM in effect at the given timestamp
	Song::BPM getBPM(Song const& song, double ts);
	/// Convert a timestamp (beats) into time (seconds); gap is the fallback offset used before any BPM is defined
	double tsTime(Song const& song, double ts, double gap);
	/// Append a BPM change at the given timestamp
	void addBPM(Song& song, double ts, float bpm, double gap);
	/// Auto-detect cover/background/video/audio files that weren't specified explicitly
	void guessFiles(Song& song);
	/// Merge per-singer vocal tracks into a combined "Together" track
	void vocalsTogether(Song& song);
	/// Post-process notes after parsing: merge duet tracks, fix overlaps/empty sentences, compute score, add beat markers
	void finalize(Song& song, unsigned tsPerBeat, unsigned tsEnd, double gap);

	/// Shared two-pass load control flow: header-only pass on first load, full notes pass once the
	/// header is already known. Also handles guessFiles, BPM recompute, and the MIDI header hook.
	void parseSong(Song& song, std::function<void(Song&)> const& parseHeader, std::function<void(Song&)> const& parseNotes);
}


