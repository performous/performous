#include "notes.hh"

#include "configuration.hh"
#include "util.hh"
#include <cmath>
#include <sstream>
#include <stdexcept>

Note::Note(): begin(getNaN()), end(getNaN()), phase(getNaN()), power(getNaN()), type(Note::Type::NORMAL), note(), notePrev() {}

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
		case Note::Type::GOLDEN:
		case Note::Type::GOLDEN2:
			return 2.0;
		case Note::Type::SLEEP:
			return 0.0;
		case Note::Type::FREESTYLE:
		case Note::Type::RAP:
		case Note::Type::NORMAL:
		case Note::Type::SLIDE:
		case Note::Type::TAP:
		case Note::Type::HOLDBEGIN:
		case Note::Type::HOLDEND:
		case Note::Type::ROLL:
		case Note::Type::MINE:
		case Note::Type::LIFT:
			return 1.0;
	}
	return 0.0;
}

double thresholdForFullScore() {
	switch(GameDifficulty(config["game/difficulty"].ui())){
		case GameDifficulty::PERFECT:
			return 0.2151;
		case GameDifficulty::HARD:
			return 0.5;
		case GameDifficulty::NORMAL:
		default:
			return 0.5;
	}
}

double thresholdForNonzeroScore() {
	switch(GameDifficulty(config["game/difficulty"].ui())){
		case GameDifficulty::PERFECT:
			return 0.5;
		case GameDifficulty::HARD:
			return 1.0;
		case GameDifficulty::NORMAL:
		default:
			return 1.5;
	}
}

float Note::powerFactor(double note) const {
	if (type == Note::Type::FREESTYLE) return 1.0;
	double error = std::abs(diff(note));
	double thresholdFull = thresholdForFullScore();
	double thresholdNonzero = thresholdForNonzeroScore();
	return static_cast<float>(clamp((thresholdNonzero - error)/(thresholdNonzero - thresholdFull), 0.0, 1.0));
}

Duration::Duration(): begin(getNaN()), end(getNaN()) {}

DanceTrack::DanceTrack(std::string& description, Notes& notes) : description(description), notes(notes) {}

VocalTrack::VocalTrack(std::string name) : name(name) {reload();}

void VocalTrack::reload() {
	notes.clear();
	m_scoreFactor = 0.0;
	noteMin = std::numeric_limits<float>::max();
	noteMax = std::numeric_limits<float>::min();
	beginTime = endTime = getNaN();
}
