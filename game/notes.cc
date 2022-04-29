#include "notes.hh"

#include "configuration.hh"
#include "util.hh"
#include <cmath>
#include <sstream>
#include <stdexcept>

Note::Note(): begin(getNaN()), end(getNaN()), phase(getNaN()), power(getNaN()), type(Note::Type::NORMAL), note(), notePrev() {}

float Note::diff(float aNote, float n) { return remainder(n - aNote, 12.0f); }
float Note::maxScore() const { return scoreMultiplier() * (end - begin); }

float Note::clampDuration(float b, float e) const {
	float len = std::min(e, end) - std::max(b, begin);
	return len > 0.0 ? len : 0.0;
}

float Note::score(float n, float b, float e) const {
	return scoreMultiplier() * powerFactor(n) * clampDuration(b, e);
}

float Note::scoreMultiplier() const {
	switch(type) {
		case Note::Type::GOLDEN:
		case Note::Type::GOLDEN2:
			return 2.0f;
		case Note::Type::SLEEP:
			return 0.0f;
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
			return 1.0f;
	}
	return 0.0f;
}

float thresholdForFullScore() {
	switch(GameDifficulty(config["game/difficulty"].i())){
		case GameDifficulty::PERFECT:
			return 0.2151f;
		case GameDifficulty::HARD:
			return 0.5f;
		case GameDifficulty::NORMAL:
		default: 
			return 0.5f;
	}
}

float thresholdForNonzeroScore() {
	switch(GameDifficulty(config["game/difficulty"].i())){
		case GameDifficulty::PERFECT:
			return 0.5f;
		case GameDifficulty::HARD:
			return 1.0f;
		case GameDifficulty::NORMAL:
		default: 
			return 1.5f;
	}
}

float Note::powerFactor(float note) const {
	if (type == Note::Type::FREESTYLE) return 1.0f;
	float error = std::abs(diff(note));
	float thresholdFull = thresholdForFullScore();
	float thresholdNonzero = thresholdForNonzeroScore();
	return clamp((thresholdNonzero - error)/(thresholdNonzero - thresholdFull), 0.0f, 1.0f);
}

Duration::Duration(): begin(getNaN()), end(getNaN()) {}

DanceTrack::DanceTrack(std::string& description, Notes& notes) : description(description), notes(notes) {}

VocalTrack::VocalTrack(std::string name) : name(name) {reload();}

void VocalTrack::reload() {
	notes.clear();
	m_scoreFactor = 0.0f;
	noteMin = std::numeric_limits<int>::max();
	noteMax = std::numeric_limits<int>::min();
	beginTime = endTime = getNaN();
}
