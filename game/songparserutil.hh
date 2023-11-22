#pragma once

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
}


