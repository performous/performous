#include "util.hh"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

//#include <boost/algorithm/string/case_conv.hpp>

// Only conversion types used in Performous are provided
template <> int sconv(std::string const& s) { return std::stoi(s); }
template <> unsigned short sconv(std::string const& s) { return static_cast<unsigned short>(std::stoi(s)); }
template <> unsigned sconv(std::string const& s) { return stou(s); }
template <> float sconv(std::string const& s) { return std::stof(s); }
template <> double sconv(std::string const& s) { return std::stod(s); }
template <> std::string sconv(std::string const& s) { return s; }

/** Converts a std::string into an unsigned int. **/
unsigned stou(std::string const & str, size_t * idx, int base) {
	std::uint64_t result = std::stoul(str, idx, base);
	unsigned uival;
	std::uint64_t mask = ~0xfffffffful;
		//Do we want this this function to throw? STL functions just overflow.
		if( (result & mask) == 0 )
			uival = static_cast<unsigned>(result);
		else throw std::out_of_range(str + " is out of range of unsigned.");
	return uival;
}

std::string format(std::chrono::seconds const& unixtime, std::string const& format, bool utc) {
	auto const time = static_cast<std::time_t>(unixtime.count());
	auto stream = std::stringstream();

	stream << std::put_time(utc ? std::gmtime(&time) : std::localtime(&time), format.c_str());

	return stream.str();
}


std::string toLower(std::string const& s) {
	//return boost::algorithm::to_lower_copy(s);
	auto result = s;

	std::for_each(result.begin(), result.end(), [](auto& c){ c = static_cast<char>(std::tolower(c));});

	return result;
}

std::string toUpper(std::string const& s) {
	//return boost::algorithm::to_upper_copy(s);
	auto result = s;

	std::for_each(result.begin(), result.end(), [](auto& c){ c = static_cast<char>(std::toupper(c));});

	return result;
}

std::string trim(std::string const& s, std::locale const& locale) {
	auto const start = std::find_if(s.begin(), s.end(), [&locale](char c){return !std::isspace(c, locale);}) - s.begin();
	auto const end = s.length() - (std::find_if(s.rbegin(), s.rend(), [&locale](char c){return !std::isspace(c, locale);}) - s.rbegin());

	return s.substr(start, end - start);
}

std::string trim(std::string& s, std::locale const& locale) {
	auto const start = std::find_if(s.begin(), s.end(), [&locale](char c){return !std::isspace(c, locale);}) - s.begin();
	auto const end = s.length() - (std::find_if(s.rbegin(), s.rend(), [&locale](char c){return !std::isspace(c, locale);}) - s.rbegin());

	s = s.substr(start, end - start);

	return s;
}
