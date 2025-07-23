#pragma once

#include "color.hh"
 
#include <string>
#include <unordered_map>
#include <vector>

struct MicrophoneConfig
{
	std::string colorname;
	Color color;
};

std::unordered_map<std::string, Color> getMicrophoneConfig();
Color getMicrophoneColor(std::string const& name);
