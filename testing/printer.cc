#include "printer.hh"

#include "game/tone.hh"

void PrintTo(Tone const& tone) {
	tone.print();
}

