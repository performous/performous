#pragma once

#include <iostream>
#include <string>

void convertToUTF8(std::stringstream &_stream, std::string _filename = std::string());
std::string convertToUTF8(std::string const& str);
std::string unicodeCollate(std::string const& str);
