// Test-only stand-ins for functions whose real implementations (configuration.cc) pull in
// dependencies (audio/graphics stack) that aren't needed to test song parsing.
#include "configuration.hh"

void populateLanguages(const std::map<std::string, std::string>&) {}
