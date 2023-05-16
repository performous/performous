#include "songparserutil.hh"

#include "unicode.hh"
#include "util.hh"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace SongParserUtil {

	void assign (int& var, std::string const& str) {
		try {
			var = std::stoi (str);
		} catch (...) {
			throw std::runtime_error ("\"" + str + "\" is not valid integer value");
		}
	}
	void assign (unsigned& var, std::string const& str) {
		try {
			var = stou(str);
		} catch (...) {
			throw std::runtime_error ("\"" + str + "\" is not valid unsigned integer value");
		}
	}
	void assign (float& var, std::string str) {
		std::replace (str.begin(), str.end(), ',', '.');  // Fix decimal separators
		try {
			var = std::stof (str);
		} catch (...) {
			throw std::runtime_error ("\"" + str + "\" is not valid floating point value");
		}
	}
	void assign (double& var, std::string str) {
		std::replace (str.begin(), str.end(), ',', '.');  // Fix decimal separators
		try {
			var = std::stod (str);
		} catch (...) {
			throw std::runtime_error ("\"" + str + "\" is not valid floating point value");
		}
	}
	void assign (bool& var, std::string const& str) {
		auto lowerStr = UnicodeUtil::toLower(str);
		auto is_yes = lowerStr == "yes" || str == "1";
		auto is_no = lowerStr == "no" || str == "0";
		if (!is_yes && !is_no) { throw std::runtime_error ("Invalid boolean value: " + str); }
		var = is_yes;
	}
	void eraseLast (std::string& s, char ch) {
		if (!s.empty() && (*s.rbegin() == ch)) {s.erase (s.size() - 1); }
	}
}

