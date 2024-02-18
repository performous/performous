#include "util.hh"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

//#include <boost/algorithm/string/case_conv.hpp>

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


std::string toLower(std::string const& s, std::locale const&) {
    //return boost::algorithm::to_lower_copy(s);
    auto result = s;

    std::for_each(result.begin(), result.end(), [](auto& c) { c = static_cast<char>(std::tolower(c)); });

    return result;
}

std::string toUpper(std::string const& s, std::locale const&) {
    //return boost::algorithm::to_upper_copy(s);
    auto result = s;

    std::for_each(result.begin(), result.end(), [](auto& c) { c = static_cast<char>(std::toupper(c)); });

    return result;
}

std::string erase(std::string const& s, std::string const& toErase) {
    auto result = s;

    return erase(result, toErase);
}

std::string erase(std::string& s, std::string const& toErase) {
    if (toErase.empty())
        return s;

    for (auto position = s.find(toErase); position < s.length(); position = s.find(toErase, position))
        s.erase(position, toErase.length());

    return s;
}

std::string replace(std::string const& s, char from, char to, bool ignoreCase) {
    auto result = s;

    return replace(result, from, to, ignoreCase);
}

std::string& replace(std::string& s, char from, char to, bool ignoreCase) {
    if (ignoreCase)
        std::replace_if(s.begin(), s.end(), [from](char c) { return tolower(c) == tolower(from); }, to);
    else
        std::replace_if(s.begin(), s.end(), [from](char c) { return c == from; }, to);

    return s;
}

std::string replace(std::string const& s, std::string const& from, std::string const& to, bool ignoreCase) {
    auto result = s;

    return replace(result, from, to, ignoreCase);
}

bool equal(std::string const& a, std::string const& b, bool ignoreCase) {
    if (ignoreCase)
        return toLower(a) == toLower(b);

    return a == b;
}

bool equal(std::string_view const& a, std::string const& b, bool ignoreCase) {
    if (!ignoreCase)
        return a == b;

    if (a.length() != b.length())
        return false;

    for (auto i = 0U; i < a.length(); ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;

    return true;
}

size_t find(std::string const& s, std::string const& toFind, size_t start, bool ignoreCase) {
    for (auto i = start; i < s.length() - toFind.length(); ++i)
        if (equal(std::string_view(s.c_str() + i, toFind.length()), toFind, ignoreCase))
            return static_cast<size_t>(i);

    return std::string::npos;
}

size_t find(std::string const& s, std::string const& toFind, bool ignoreCase) {
    return find(s, toFind, 0U, ignoreCase);
}

std::string& replace(std::string& s, std::string const& from, std::string const& to, bool ignoreCase) {
    for (auto position = find(s, from, ignoreCase); position < s.length(); position = find(s, from, position, ignoreCase)) {
        s.erase(position, from.length());
        s.insert(position, to);
    }

    return s;
}

std::string substr(std::string const& s, ptrdiff_t start, ptrdiff_t length = -1) {
    return s.substr(static_cast<size_t>(start), static_cast<size_t>(length));
}

std::string trim(std::string const& s, std::locale const& locale) {
    auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();
    auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

    return substr(s, start, end - start);
}

std::string& trim(std::string& s, std::locale const& locale) {
    auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();
    auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

    s = substr(s, start, end - start);

    return s;
}

std::string trimFront(std::string const& s, std::locale const& locale) {
    auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();

    return substr(s, start);
}

std::string& trimFront(std::string& s, std::locale const& locale) {
    auto const start = std::find_if(s.begin(), s.end(), [&locale](char c) {return !std::isspace(c, locale); }) - s.begin();

    s = substr(s, start);

    return s;
}

std::string trimBack(std::string const& s, std::locale const& locale) {
    auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

    return substr(s, 0, end);
}

std::string& trimBack(std::string& s, std::locale const& locale) {
    auto const end = static_cast<ptrdiff_t>(s.length()) - (std::find_if(s.rbegin(), s.rend(), [&locale](char c) {return !std::isspace(c, locale); }) - s.rbegin());

    s = substr(s, 0, end);

    return s;
}

bool startsWith(std::string const& s, std::string const& start) {
    return std::string_view(s).substr(0, start.length()) == start;
}
