#include "util.hh"

#include <cmath>
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

bool isNaN(float f) {
#ifdef _WIN32
	return _isnanf(f);
#elif defined(__APPLE__)
	return isnan(f);
#else
	return isnanf(f);
#endif
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

std::string format(std::chrono::seconds const& unixtime, std::string const& format, bool utc) {
    auto const time = static_cast<std::time_t>(unixtime.count());
    auto stream = std::stringstream();

    stream << std::put_time(utc ? std::gmtime(&time) : std::localtime(&time), format.c_str());

    return stream.str();
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
