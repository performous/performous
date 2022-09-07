#include "printer.hh"

#include "game/tone.hh"

void PrintTo(Tone const& tone, std::ostream* os) {
	tone.print(*os);
}

