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

bool startsWithUTF8BOM(std::string const& s) {
    if (s.size() < 3)
        return false;
    auto const b0 = static_cast<unsigned char>(s[0]);
    auto const b1 = static_cast<unsigned char>(s[1]);
    auto const b2 = static_cast<unsigned char>(s[2]);
    if (b0 == 0xEF && b1 == 0xBB && b2 == 0xBF)
        return true;

    return false;
}

bool isText(std::string const& s, size_t bytesToCheck) {
    enum ByteType { ASCII, Start, Follow };
    auto const start = startsWithUTF8BOM(s) ? 3U : 0U;
    auto const end = std::min(bytesToCheck, s.size());
    auto byteType = ASCII;
    auto follow = 0;

    for (auto n = start; n < end; ++n) {
        auto const unsignedChar = static_cast<unsigned char>(s[n]);

        if ((unsignedChar & 0xC0) == 0xC0) {
            if ((unsignedChar & 0xF0) == 0xF0 && (unsignedChar & 0x0F) > 4)
                return false;
            byteType = Start;
            follow = 0;
        }
        else if ((unsignedChar & 0x80) == 0x80) {
            if (byteType == ASCII)
                return false;
            if (++follow == 4)
                return false;
            byteType = Follow;
        }
        else {
            if (s[n] < 32 && s[n] != '\n' && s[n] != '\r' && s[n] != '\t')
                return false;
            byteType = ASCII;
        }
    }

    return true;
}
