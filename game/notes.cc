﻿#include "notes.hh"

#include "configuration.hh"
#include "util.hh"
#include <cmath>
#include <sstream>
#include <stdexcept>

Note::Note(): begin(getNaN()), end(getNaN()), phase(getNaN()), power(getNaN()), type(NORMAL), note(), notePrev() {}

double Note::diff(double note, double n) { return remainder(n - note, 12.0); }
double Note::maxScore() const { return scoreMultiplier() * (end - begin); }

double Note::clampDuration(double b, double e) const {
	double len = std::min(e, end) - std::max(b, begin);
	return len > 0.0 ? len : 0.0;
}

double Note::score(double n, double b, double e) const {
	return scoreMultiplier() * powerFactor(n) * clampDuration(b, e);
}

double Note::scoreMultiplier() const {
	switch(type) {
		case GOLDEN:
		case GOLDEN2:
			return 2.0;
		case SLEEP:
			return 0.0;
		case FREESTYLE:
		case RAP:
		case NORMAL:
		case SLIDE:
		case TAP:
		case HOLDBEGIN:
		case HOLDEND:
		case ROLL:
		case MINE:
		case LIFT:
			return 1.0;
	}
	return 0.0;
}

double Note::powerFactor(double note) const {
	if (type == FREESTYLE) return 1.0;
	double error = std::abs(diff(note));
	switch(config["game/difficulty"].i()){
		case 0: // normal mode (original)
			//no points if error> 150 cents, perfect score if error <50 cents
			return clamp(1.5 - error, 0.0, 1.0);
		case 1: // "harder mode"
			//no points if error> 50 cents, perfect score if error < 21.51 cents (syntonic comma)
			return clamp((0.5-error)/(0.5-0.2151), 0.0, 1.0);
		case 2: // "perfect mode"
			//no points if error>21.51 cents, perfect score if error < 6 cents
			return clamp((0.2151-error)/(0.2151-0.06), 0.0, 1.0); 
	}
	return clamp(1.5 - error, 0.0, 1.0);
}

Duration::Duration(): begin(getNaN()), end(getNaN()) {}

DanceTrack::DanceTrack(std::string& description, Notes& notes) : description(description), notes(notes) {}

VocalTrack::VocalTrack(std::string name) : name(name) {reload();}

void VocalTrack::reload() {
	notes.clear();
	m_scoreFactor = 0.0;
	noteMin = std::numeric_limits<int>::max();
	noteMax = std::numeric_limits<int>::min();
	beginTime = endTime = getNaN();
}
