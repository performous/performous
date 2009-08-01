#pragma once

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "engine.hh"
#include "joystick.hh"
#include "surface.hh"
#include "opengl_text.hh"

class Song;

struct Chord {
	double begin, end;
	bool fret[5];
	Duration const* dur[5];
	int polyphony;
	bool tappable;
	int status; // 0 = not played, 1 = tapped, 2 = picked, 3 = released
	int score;
	Chord(): begin(), end(), polyphony(), tappable(), status(), score() {
		std::fill(fret, fret + 5, false);
		std::fill(dur, dur + 5, static_cast<Duration const*>(NULL));
	}
	bool matches(bool const* fretPressed) {
		for (int i = 0; i < 5; ++i) {
			std::cout << fret[i] << fretPressed[i] << " ";
		}
		std::cout << std::endl;
		if (polyphony == 1) {
			bool shadowed = true;
			for (int i = 0; i < 5; ++i) {
				if (fret[i]) shadowed = false;
				if (!shadowed && fret[i] != fretPressed[i]) return false;
			}
			return true;
		}
		return std::equal(fret, fret + 5, fretPressed);
	}
};

static inline bool operator==(Chord const& a, Chord const& b) {
	return std::equal(a.fret, a.fret + 5, b.fret);
}

/// handles drawing of notes and waves
class GuitarGraph {
  public:
	/// constructor
	GuitarGraph(Song const& song, bool drums = false);
	/** draws GuitarGraph
	 * @param time at which time to draw
	 */
	void draw(double time);
	void engine(double time);
	void position(double cx, double width) { m_cx = cx; m_width = width; }
  private:
	input::InputDev m_input;
	Song const& m_song;
	Surface m_button;
	Surface m_tap;
	AnimValue m_pickValue;
	boost::ptr_vector<Texture> m_necks;
	bool m_drums;
	double m_cx, m_width;
	std::size_t m_instrument;
	void drumHit(double time, int pad);
	void guitarPlay(double time, input::Event const& ev);
	enum Difficulty {
		DIFFICULTY_SUPAEASY,
		DIFFICULTY_EASY,
		DIFFICULTY_MEDIUM,
		DIFFICULTY_AMAZING,
		DIFFICULTYCOUNT
	} m_level;
	glutil::Color const& color(int fret) const;
	void drawBar(double time, float h);
	void difficultyAuto();
	bool difficulty(Difficulty level);
	SvgTxtTheme m_text;
	void updateChords();
	typedef std::vector<Chord> Chords;
	Chords m_chords;
	Chords::iterator m_chordIt;
	typedef std::map<Duration const*, int> NoteStatus;
	NoteStatus m_notes;
	AnimValue m_hammerReady;
	int m_score;
};

