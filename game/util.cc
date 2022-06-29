#include "util.hh"

// Only conversion types used in Performous are provided
template <> int sconv(std::string const& s) { return std::stoi(s); }
template <> unsigned int sconv(std::string const& s) { return std::stoul(s); }
template <> float sconv(std::string const& s) { return std::stof(s); }
template <> double sconv(std::string const& s) { return std::stod(s); }
template <> std::string sconv(std::string const& s) { return s; }
