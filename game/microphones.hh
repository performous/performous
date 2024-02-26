#pragma once

#include "color.hh"

#include <string>
#include <vector>

struct MicrophoneConfig
{
    std::string colorname;
    Color color;
};

std::vector<MicrophoneConfig> getMicrophoneConfig();
Color getMicrophoneColor(std::string const& name);
