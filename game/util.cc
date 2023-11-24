#include "util.hh"

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

std::string reverse(const std::string& s) {
	auto result = std::string{};

	for (auto i = size_t(0), j = s.size() - 1; i < s.size(); ++i, --j)
		result += s[j];

	return result;
}

std::string trim(const std::string& s, const std::string& charactersToRemove) {
	auto result = s;
	const auto start = result.find_first_not_of(charactersToRemove);

	if (start == std::string::npos)
		return {};

	result = result.substr(start);
	result = reverse(result);

	const auto end = result.find_first_not_of(charactersToRemove);

	if (end != std::string::npos)
		result = result.substr(end);

	return reverse(result);
}
