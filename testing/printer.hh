#pragma once

#include <ostream>

struct Tone;

void PrintTo(Tone const& tone, std::ostream* os);
