#include "util.hh"

#include <fmt/chrono.h>

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Only conversion types used in Performous are provided
template <> int sconv(std::string const& s) { return std::stoi(s); }
template <> unsigned short sconv(std::string const& s) { return static_cast<unsigned short>(std::stoi(s)); }
template <> unsigned sconv(std::string const& s) { return stou(s); }
template <> float sconv(std::string const& s) { return std::stof(s); }
template <> double sconv(std::string const& s) { return std::stod(s); }
template <> std::string sconv(std::string const& s) { return s; }

bool containsNoCase(std::string const& word, std::string const& part) {
	return toLower(word).find(toLower(part)) != std::string::npos;
}

/** Converts a std::string into an unsigned int. **/
unsigned stou(std::string const& str, size_t* idx, int base) {
	std::uint64_t result = std::stoul(str, idx, base);
	unsigned uival;
	std::uint64_t mask = ~0xfffffffful;
	//Do we want this this function to throw? STL functions just overflow.
	if ((result & mask) == 0)
		uival = static_cast<unsigned>(result);
	else throw std::out_of_range(str + " is out of range of unsigned.");
	return uival;
}

std::string timeFormat(std::chrono::seconds const& unixtime, std::string const& format_spec, bool utc) {
	auto const tp = std::chrono::system_clock::time_point(unixtime);
	std::string fmt_string{fmt::format("{{:{}}}", format_spec)};
	if (utc) {
		return fmt::format(fmt::runtime(fmt_string), fmt::gmtime(tp));
	}
	else {
		const auto tt = std::chrono::system_clock::to_time_t(tp);
#ifdef _WIN32
		struct tm ltime;
		errno_t err = localtime_s(&ltime, &tt);
		if (err != EINVAL) {
			return fmt::format(fmt::runtime(fmt_string), ltime);
		}
		else {
		// If local-time conversion fails, just return utc time, no big deal.
			return fmt::format(fmt::runtime(fmt_string), fmt::gmtime(tp));
		}
#else
		struct tm ltime;
		localtime_r(&tt, &ltime);
		if (errno != EOVERFLOW) {
			return fmt::format(fmt::runtime(fmt_string), ltime);
		}
			return fmt::format(fmt::runtime(fmt_string), fmt::gmtime(tp));
#endif
	}
}

bool isSingleByteCharacter(char c) {
	return (static_cast<unsigned char>(c) & 0b10000000) == 0;
}

bool isTwoByteCharacter(char c) {
	return (static_cast<unsigned char>(c) & 0b11100000) == 0b11000000;
}

bool isThreeByteCharacter(char c) {
	return (static_cast<unsigned char>(c) & 0b11110000) == 0b11100000;
}

bool isFourByteCharacter(char c) {
	return (static_cast<unsigned char>(c) & 0b11111000) == 0b11110000;
}

bool isInvalidFollowByte(char c) {
	return (static_cast<unsigned char>(c) & 0b11000000) != 0b10000000;
}

bool isASCII(char c) {
	return static_cast<unsigned char>(c) >= 32 || std::isspace(static_cast<unsigned char>(c));
}

bool isText(const std::string& text, size_t bytesToCheck) {
	bytesToCheck = std::min(bytesToCheck, text.size());
	for (size_t i = 0; i < bytesToCheck; ++i) {
		if (isSingleByteCharacter(text[i])) {
			if (!isASCII(text[i]))
				return false;
		}
		else if (isTwoByteCharacter(text[i])) {
			if (i + 1 >= text.size() || isInvalidFollowByte(text[++i]))
				return false;
		}
		else if (isThreeByteCharacter(text[i])) {
			if (i + 2 >= text.size() || isInvalidFollowByte(text[++i]) || isInvalidFollowByte(text[++i]))
				return false;
		}
		else if (isFourByteCharacter(text[i])) {
			if (i + 3 >= text.size() || isInvalidFollowByte(text[++i]) || isInvalidFollowByte(text[++i]) || isInvalidFollowByte(text[++i]))
				return false;
		}
		else // Invalid UTF-8 sequence
			return false;
	}

	return true;
}

std::string toLower(std::string const& s) {
	//return boost::algorithm::to_lower_copy(s);
	auto result = s;

	std::for_each(result.begin(), result.end(), [](auto& c) { c = static_cast<char>(std::tolower(c)); });

	return result;
}

std::string toUpper(std::string const& s) {
	//return boost::algorithm::to_upper_copy(s);
	auto result = s;

	std::for_each(result.begin(), result.end(), [](auto& c) { c = static_cast<char>(std::toupper(c)); });

	return result;
}

std::string replace(std::string const& s, char from, char to) {
	auto result = s;

	std::replace_if(result.begin(), result.end(), [from](char c) {return c == from; }, to);

	return result;
}

std::string& replace(std::string& s, char from, char to) {
	std::replace_if(s.begin(), s.end(), [from](char c) {return c == from; }, to);

	return s;
}

std::string substr(std::string const& s, ptrdiff_t start) {
	return s.substr(static_cast<std::size_t>(start));
}

template<class Type>
std::string substr(std::string const& s, ptrdiff_t start, Type end) {
	return s.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(static_cast<ptrdiff_t>(end) - start));
}

std::string trim(std::string const& s, std::locale const& locale) {
	auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();
	auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

	return substr(s, start, end);
}

std::string& trim(std::string& s, std::locale const& locale) {
	auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();
	auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

	s = substr(s, start, end);

	return s;
}

std::string trimLeft(std::string const& s, std::locale const& locale) {
	auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();

	return substr(s, start);
}

std::string& trimLeft(std::string& s, std::locale const& locale) {
	auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();

	s = substr(s, start);

	return s;
}

std::string trimRight(std::string const& s, std::locale const& locale) {
	auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

	return substr(s, 0, end);
}

std::string& trimRight(std::string& s, std::locale const& locale) {
	auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

	s = substr(s, 0, end);

	return s;
}

std::string replaceFirst(std::string const& s, std::string const& from, std::string const& to) {
	auto const position = s.find(from);

	if (position == std::string::npos)
		return s;

	auto result = s;

	result.replace(position, from.length(), to);

	return result;
}
