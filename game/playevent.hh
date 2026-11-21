#pragma once

#include "animvalue.hh"
#include "notes.hh"
#include "util.hh"

const unsigned max_panels = 10; // Maximum number of arrow lines / guitar frets

/// A note played (or missed) by an instrument player
struct PlayEvent {
	double time;
	AnimValue glow;
	AnimValue whammy;
	unsigned type; // 0 = miss (pick), 1 = tap, 2 = pick
	int fret;
	Duration const* dur;
	double holdTime;
	PlayEvent(double t, unsigned ty, int f = -1, Duration const* d = nullptr): time(t), glow(0.0, 5.0), whammy(0.0, 0.5), type(ty), fret(f), dur(d), holdTime(d ? d->begin : getNaN()) { if (type > 0) glow.setValue(1.0); }
};
