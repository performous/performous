#include "guitargraph.hh"

#include "fs.hh"
#include "song.hh"
#include "3dobject.hh"
#include <cmath>
#include <cstdlib>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

namespace {
	struct Diff { std::string name; int basepitch; } diffv[] = {
		{ "Easy", 0x3C },
		{ "Medium", 0x48 },
		{ "Hard", 0x54 },
		{ "Expert", 0x60 }
	};
	const size_t diffsz = sizeof(diffv) / sizeof(*diffv);
	const int death_delay = 20; // Delay in notes after which the player is hidden
	const float join_delay = 6.0f; // Time to select track/difficulty when joining mid-game
	const float g_angle = 80.0f;
	const float past = -0.2f;
	const float future = 1.5f;
	const float timescale = 25.0f;
	const float texCoordStep = -0.5f; // Two beat lines per neck texture => 0.5 tex units per beat
	const float flameSpd = 6.0f; // Multiplier for flame growth
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return -timescale * (t - past) / (future - past); }
	float time2a(float t) {
		float a = clamp(1.0 - t / future); // Note: we want 1.0 alpha already at zero t.
		return std::pow(a, 0.8f); // Nicer curve
	}
	float y2a(float y) { return time2a(past - y / timescale * (future - past)); }
	const double maxTolerance = 0.15;
	
	double points(double error) {
		double score = 0.0;
		if (error < maxTolerance) score += 15;
		if (error < 0.1) score += 15;
		if (error < 0.05) score += 15;
		if (error < 0.03) score += 5;
		return score;
	}
	
	const int streakStarBonus = 500;
	int getNextBigStreak(int prev) { return prev + 50; }
	inline float blend(float a, float b, float f) { return a*f + b*(1.0f-f); }
	
	glutil::Color starpowerColorize(glutil::Color c, float f) {
		static glutil::Color starpowerC(0.5f, 0.5f, 1.0f);
		if ( f < 0.001 ) return c;
		f = std::sqrt(std::sqrt(f));
		c.r = blend(starpowerC.r, c.r, f);
		c.g = blend(starpowerC.g, c.g, f);
		c.b = blend(starpowerC.b, c.b, f);
		return c;
	}
}

GuitarGraph::GuitarGraph(Audio& audio, Song const& song, bool drums, int number):
  m_audio(audio),
  m_input(drums ? input::DRUMS : input::GUITAR),
  m_song(song),
  m_button(getThemePath("button.svg")),
  m_button_l(getThemePath("button_l.svg")),
  m_flame(getThemePath("flame.svg")),
  m_flame_godmode(getThemePath("flame_godmode.svg")),
  m_tap(getThemePath("tap.svg")),
  m_neckglow(getThemePath("neck_glow.svg")),
  m_neckglowColor(),
  m_drums(drums),
  m_use3d(config["graphic/3d_notes"].b()),
  m_starpower(0.0, 0.1),
  m_cx(0.0, 0.2),
  m_width(0.5, 0.4),
  m_stream(),
  m_track_index(m_track_map.end()),
  m_level(),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_correctness(0.0, 5.0),
  m_streakPopup(0.0, 1.0),
  m_godmodePopup(0.0, 0.666),
  m_score(),
  m_scoreFactor(),
  m_starmeter(),
  m_streak(),
  m_longestStreak(),
  m_bigStreak(),
  m_jointime(getNaN()),
  m_dead()
{
	// Copy all tracks of supported types (either drums or non-drums) to m_track_map
	for (TrackMap::const_iterator it = m_song.track_map.begin(); it != m_song.track_map.end(); ++it) {
		std::string index = it->first;
		if (m_drums == (index == "drums")) m_track_map[index] = &it->second;
	}
	if (m_track_map.empty()) throw std::logic_error(m_drums ? "No drum tracks found" : "No guitar tracks found");
	// Load 3D fret objects
	m_fretObj.load(getThemePath("fret.obj"));
	m_tappableObj.load(getThemePath("fret_tap.obj"));
	// Score calculator (TODO a better one)
	m_popupText.reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	// Load fail sounds
	unsigned int sr = m_audio.getSR();
	if (m_drums) {
		m_samples.push_back(Sample(getPath("sounds/drum_bass.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/drum_snare.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/drum_hi-hat.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/drum_tom1.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/drum_cymbal.ogg"), sr));
		//m_samples.push_back(Sample(getPath("sounds/drum_tom2.ogg"), sr));
	} else {
		m_samples.push_back(Sample(getPath("sounds/guitar_fail1.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/guitar_fail2.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/guitar_fail3.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/guitar_fail4.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/guitar_fail5.ogg"), sr));
		m_samples.push_back(Sample(getPath("sounds/guitar_fail6.ogg"), sr));
	}
	for (int i = 0; i < 6; ++i) m_hit[i].setRate(5.0);
	for (int i = 0; i < 5; ++i) m_holds[i] = 0;
	m_track_index = m_track_map.begin();
	while (number--) nextTrack(true);
	difficultyAuto();
	updateNeck();
}

/// Load the appropriate neck texture
void GuitarGraph::updateNeck() {
	// TODO: Optimize with texture cache
	std::string index = m_track_index->first;
	if (index == "drums") m_neck.reset(new Texture(getThemePath("drumneck.svg")));
	else if (index == "bass") m_neck.reset(new Texture(getThemePath("bassneck.svg")));
	else m_neck.reset(new Texture(getThemePath("guitarneck.svg")));
}

/// Cycle through the different tracks
void GuitarGraph::nextTrack(bool fast) {
	if (++m_track_index == m_track_map.end()) m_track_index = m_track_map.begin();
	if (fast) return;
	difficultyAuto(true);
	updateNeck();
}

/// Get the difficulty as displayable string
std::string GuitarGraph::getDifficultyString() const {
	std::string ret = diffv[m_level].name;
	if (m_drums && m_input.isKeyboard()) ret += " (kbd)";
	return ret;
}

/// Find an initial difficulty level to use
void GuitarGraph::difficultyAuto(bool tryKeep) {
	if (tryKeep && difficulty(Difficulty(m_level))) return;
	for (int level = 0; level < DIFFICULTYCOUNT; ++level) if (difficulty(Difficulty(level))) return;
	throw std::runtime_error("No difficulty levels found");
}

/// Attempt to use a given difficulty level
bool GuitarGraph::difficulty(Difficulty level) {
	Track const& track = *m_track_index->second;
	// Find the stream number
	for (TrackMap::const_iterator it = m_song.track_map.begin(); it != m_song.track_map.end(); ++it) {
		if (&track == &it->second) break;
	}
	// Check if the difficulty level is available
	uint8_t basepitch = diffv[level].basepitch;
	NoteMap const& nm = track.nm;
	int fail = 0;
	for (int fret = 0; fret < 5; ++fret) if (nm.find(basepitch + fret) == nm.end()) ++fail;
	if (fail == 5) return false;
	Difficulty prevLevel = m_level;
	m_level = level;
	updateChords();
	if (m_chords.size() <= 1) { // If there is only one chord, it's probably b0rked
		m_level = prevLevel;
		updateChords();
		return false;
	}
	return true;
}


/// Core engine
void GuitarGraph::engine() {
	double time = m_audio.getPosition();
	time -= config["audio/controller_delay"].f();
	// Handle key markers
	if (!m_drums) {
		for (int i = 0; i < 5; ++i) {
			if (m_input.pressed(i)) m_hit[i + 1].setValue(1.0);
		}
	}
	if (m_starpower.get() > 0.001) m_correctness.setTarget(1.0, true);
	double whammy = 0;
	bool difficulty_changed = false;
	// Handle all events
	for (input::Event ev; m_input.tryPoll(ev);) {
		// This hack disallows joining with Enter-key for skipping instrumental
		// breaks to be usable with FoF songs.
		if (dead() && m_input.isKeyboard() && ev.type == input::Event::PICK) continue;
		m_dead = 0; // Keep alive
		if (m_jointime != m_jointime) m_jointime = time < -1.0 ? -1.0 : time + join_delay; // Handle joining
		// Handle Start/Select keypresses
		if (ev.type == input::Event::PRESS && ev.button > input::STARPOWER_BUTTON) {
			if (ev.button == 9) ev.button = input::STARPOWER_BUTTON; // Start works for GodMode
			else continue;
		}
		// Guitar specific actions
		if (!m_drums) {
			if ((ev.type == input::Event::PRESS || ev.type == input::Event::RELEASE) && ev.button == input::STARPOWER_BUTTON) {
				if (ev.type == input::Event::PRESS) activateStarpower();
				continue;
			}
			if (ev.type == input::Event::RELEASE) endHold(ev.button, time);
			if (ev.type == input::Event::WHAMMY) whammy = (1.0 + ev.button + 2.0*(rand()/double(RAND_MAX))) / 4.0;
		}
		// Keypress anims
		if (ev.type == input::Event::PRESS) m_hit[!m_drums + ev.button].setValue(1.0);
		else if (ev.type == input::Event::PICK) m_hit[0].setValue(1.0);
		// Difficulty and track selection
		if (time < m_jointime) {
			if (ev.type == input::Event::PICK || ev.type == input::Event::PRESS) {
				if (!m_drums && ev.pressed[4]) nextTrack();
				if (ev.pressed[0 + m_drums]) difficulty(DIFFICULTY_SUPAEASY);
				else if (ev.pressed[1 + m_drums]) difficulty(DIFFICULTY_EASY);
				else if (ev.pressed[2 + m_drums]) difficulty(DIFFICULTY_MEDIUM);
				else if (ev.pressed[3 + m_drums]) difficulty(DIFFICULTY_AMAZING);
				difficulty_changed = true;
			}
		// Playing
		} else if (m_drums) {
			if (ev.type == input::Event::PRESS) drumHit(time, ev.button);
		} else {
			guitarPlay(time, ev);
		}
		if (m_score < 0) m_score = 0;
	}
	// Skip missed notes
	while (m_chordIt != m_chords.end() && m_chordIt->begin + maxTolerance < time) {
		if ( (m_drums && m_chordIt->status != m_chordIt->polyphony) 
		  || (!m_drums && m_chordIt->status == 0) ) endStreak();
		++m_dead;
		++m_chordIt;
	}
	if (difficulty_changed) m_dead = 0; // if difficulty is changed, m_dead would get incorrect
	// Adjust the correctness value
	if (!m_events.empty() && m_events.back().type == 0) m_correctness.setTarget(0.0, true);
	else if (m_chordIt != m_chords.end() && m_chordIt->begin <= time) {
		double level;
		if (m_drums) level = double(m_chordIt->status) / m_chordIt->polyphony;
		else level = m_chordIt->status ? 1.0 : 0.0;
		m_correctness.setTarget(level);
	}
	if (m_correctness.get() == 0) endStreak();
	// Process holds
	if (!m_drums) {
		for (int fret = 0; fret < 5; ++fret) {
			if (!m_holds[fret]) continue;
			Event& ev = m_events[m_holds[fret] - 1];
			ev.glow.setTarget(1.0, true);
			// Whammy animvalue mangling
			ev.whammy.setTarget(whammy);
			if (whammy > 0) {
				ev.whammy.move(0.5);
				if (ev.whammy.get() > 1.0) ev.whammy.setValue(1.0);
			}
			// Calculate how long the note was held this cycle and score it
			double last = std::min(time, ev.dur->end);
			double t = last - ev.holdTime;
			if (t > 0) {
				// No points for long holds unless whammy is used
				double wfactor = (time - ev.dur->begin < 1.5 || ev.whammy.get() > 0.01
				  || m_starpower.get() > 0.01) ? 1.0 : 0.0;
				m_score += t * 50.0 * wfactor;
				// Whammy fills starmeter much faster
				m_starmeter += t * 50 * ( (ev.whammy.get() > 0.01) ? 2.0 : 1.0 );
				ev.holdTime = time;
			}
			// If end reached, handle it
			if (last == ev.dur->end) endHold(fret, time);
		}
	}
	// Check if a long streak goal has been reached
	if (m_streak >= getNextBigStreak(m_bigStreak)) {
		m_streakPopup.setTarget(1.0);
		m_bigStreak = getNextBigStreak(m_bigStreak);
		m_starmeter += streakStarBonus;
	}
	// During GodMode, correctness is full, no matter what
	if (m_starpower.get() > 0.01) m_correctness.setTarget(1.0, true);
}

/// Are we alive?
bool GuitarGraph::dead() const {
	return m_jointime != m_jointime || m_dead >= death_delay;
}

/// Attempt to activate GodMode
void GuitarGraph::activateStarpower() {
	if (canActivateStarpower()) {
		m_starmeter = 0;
		m_starpower.setValue(1.0);
		m_godmodePopup.setTarget(1.0);
	}
}

/// Mark the holding of a note as ended
void GuitarGraph::endHold(int fret, double time) {
	if (fret >= 5 || !m_holds[fret]) return;
	m_events[m_holds[fret] - 1].glow.setTarget(0.0);
	m_events[m_holds[fret] - 1].whammy.setTarget(0.0, true);
	m_holds[fret] = 0;
	if (time > 0) { // Do we set the relaseTime?
		// Search for the Chord this hold belongs to
		for (Chords::iterator it = m_chords.begin(); it != m_chords.end(); ++it) {
			if (time >= it->begin && time <= it->end) {
				it->releaseTimes[fret] = time;
				if (it->status < 100 && time >= it->end - maxTolerance)
					it->status += 100; // Mark as past note for rewinding
				break;
			}
		}
	}
}

/// Do stuff when a note is played incorrectly
void GuitarGraph::fail(double time, int fret) {
	if (fret == -2) return; // Tapped note
	if (fret == -1) {
		for (int i = 0; i < 5; ++i) endHold(i, time);
	}
	if (m_starpower.get() < 0.01) {
		// Reduce points and play fail sample only when GodMode is deactivated
		m_events.push_back(Event(time, 0, fret));
		if (fret < 0) fret = std::rand();
		m_audio.play(m_samples[unsigned(fret) % m_samples.size()], "audio/fail_volume");
		m_score -= 50;
	}
	endStreak();
}

/// Handle drum hit scoring
void GuitarGraph::drumHit(double time, int fret) {
	if (fret >= 5) return;
	// Find any suitable note within the tolerance
	double tolerance = maxTolerance;
	Chords::iterator best = m_chords.end();
	for (Chords::iterator it = m_chordIt; it != m_chords.end() && it->begin <= time + tolerance; ++it) {
		if (m_notes[it->dur[fret]]) continue;  // Already played
		double error = std::abs(it->begin - time);
		if (error < tolerance) {
			best = it;
			tolerance = error;
		}
	}
	if (best == m_chords.end()) fail(time, fret);
	else {
		for (; best != m_chordIt; ++m_chordIt) {
			if (m_chordIt->status == m_chordIt->polyphony) continue;
			endStreak();
		}
		++m_chordIt->status;
		Duration const* dur = m_chordIt->dur[fret];
		m_events.push_back(Event(time, 1, fret, dur));
		m_notes[dur] = m_events.size();
		double score = points(tolerance);
		m_chordIt->score += score;
		m_score += score;
		m_flames[fret].push_back(AnimValue(0.0, flameSpd));
		m_flames[fret].back().setTarget(1.0);
		if (m_chordIt->status == m_chordIt->polyphony) {
			//m_score -= m_chordIt->score;
			//m_chordIt->score *= m_chordIt->polyphony;
			//m_score += m_chordIt->score;
			m_streak += 1;
			if (m_streak > m_longestStreak) m_longestStreak = m_streak;
		}
		m_correctness.setTarget(double(m_chordIt->status) / m_chordIt->polyphony, true);
	}
}

/// Handle guitar events and scoring
void GuitarGraph::guitarPlay(double time, input::Event const& ev) {
	bool picked = (ev.type == input::Event::PICK);
	bool frets[5] = {};  // The combination about to be played
	if (picked) {
		for (int fret = 0; fret < 5; ++fret) {
			frets[fret] = ev.pressed[fret];
		}
	} else { // Attempt to tap
		if (ev.button >= 5) return;
		if (m_correctness.get() < 0.5 && m_starpower.get() < 0.001) return; // Hammering not possible at the moment
		for (int fret = ev.button + 1; fret < 5; ++fret) {
			if (ev.pressed[fret]) return; // Extra buttons on right side
		}
		if (ev.type == input::Event::PRESS) {
			// Hammer-on, the fret pressed is played
			frets[ev.button] = true;
		} else {
			// Pull off, find the note to played that way
			int fret = ev.button;
			do {
				if (--fret < 0) return; // No frets pressed -> not a pull off
			} while (!ev.pressed[fret]);
			frets[fret] = true;
		}
	}
	// Find any suitable note within the tolerance
	double tolerance = maxTolerance;
	Chords::iterator best = m_chords.end();
	for (Chords::iterator it = m_chordIt; it != m_chords.end() && it->begin <= time + tolerance; ++it) {
		if (it->status > 1) continue; // Already picked, can't play again
		if (!picked) { // Tapping rules
			if (it->status > 0) continue; // Already tapped, can't tap again
			if (!it->tappable) continue; // Cannot tap
			Chords::iterator tmp = it;
			if ( (tmp == m_chords.begin() || (--tmp)->status == 0) &&
			  m_starpower.get() < 0.001) continue; // The previous note not played
		}
		if (!it->matches(frets)) continue;
		double error = std::abs(it->begin - time);
		if (error < tolerance) {
			best = it;
			tolerance = error;
		}
	}
	if (best == m_chords.end()) fail(time, picked ? -1 : -2);
	else { // Correctly hit
		m_chordIt = best;
		int& score = m_chordIt->score;
		m_score -= score;
		m_starmeter -= score;
		bool first_time = (score == 0 ? true : false);
		if (first_time) m_streak++;
		if (m_streak > m_longestStreak) m_longestStreak = m_streak;
		score = points(tolerance);
		score *= m_chordIt->polyphony;
		m_chordIt->status = 1 + picked;
		m_score += score;
		m_starmeter += score;
		m_correctness.setTarget(1.0, true); // Instantly go to one
		for (int fret = 0; fret < 5; ++fret) {
			if (!m_chordIt->fret[fret]) continue;
			Duration const* dur = m_chordIt->dur[fret];
			m_events.push_back(Event(time, 1 + picked, fret, dur));
			m_notes[dur] = m_events.size();
			m_holds[fret] = m_events.size();
			if (first_time) {
				m_flames[fret].push_back(AnimValue(0.0, flameSpd));
				m_flames[fret].back().setTarget(1.0);
			}
		}
	}
}

/// Get a color based on fret index
glutil::Color const& GuitarGraph::color(int fret) const {
	static glutil::Color fretColors[5] = {
		glutil::Color(0.0f, 0.9f, 0.0f),
		glutil::Color(0.9f, 0.0f, 0.0f),
		glutil::Color(0.9f, 0.9f, 0.0f),
		glutil::Color(0.0f, 0.0f, 1.0f),
		glutil::Color(0.9f, 0.4f, 0.0f)
	};
	if (fret < 0 || fret > 4) throw std::logic_error("Invalid fret number in GuitarGraph::getColor");
	if (m_drums) {
		if (fret == 0) fret = 4;
		else if (fret == 4) fret = 0;
	}
	return fretColors[fret];
}

namespace {
	const float fretWid = 0.5f; // The actual width is two times this
	
	/// Create a symmetric vertex pair of given data
	void vertexPair(float x, float y, glutil::Color c, float ty, float fretW = fretWid) {
		c.a = y2a(y); glColor4fv(c);
		glNormal3f(0.0f,1.0f,0.0f); glTexCoord2f(0.0f, ty); glVertex2f(x - fretW, y);
		glNormal3f(0.0f,1.0f,0.0f); glTexCoord2f(1.0f, ty); glVertex2f(x + fretW, y);
	}
}

/// Main drawing function (projection, neck, cursor...)
void GuitarGraph::draw(double time) {
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenBottom().middle(m_cx.get()).fixedWidth(m_width.get());
	double offsetX = 0.5 * (dimensions.x1() + dimensions.x2());
	double frac = 0.75;  // Adjustable: 1.0 means fully separated, 0.0 means fully attached
	float ng_r = 0, ng_g = 0, ng_b = 0; // neck glow color components
	int ng_ccnt = 0; // neck glow color count
	{	// Translate, rotate and scale to place
		// The scope blocks are there so that the actions are automatically reversed
		glutil::PushMatrixMode pmm(GL_PROJECTION); {
		glutil::Translation tr1(frac * 2.0 * offsetX, 0.0f, 0.0f); {
		glutil::PushMatrixMode pmb(GL_MODELVIEW); {
		glutil::Translation tr2((1.0 - frac) * offsetX, dimensions.y2(), 0.0f); {
		glutil::Rotation rot1(g_angle, 1.0f, 0.0f, 0.0f); {
		float temp_s = dimensions.w() / 5.0f;
		glutil::Scale sc1(temp_s, temp_s, temp_s);
		
			// Draw the neck
			{
				UseTexture tex(*m_neck);
				glutil::Begin block(GL_TRIANGLE_STRIP);
				float w = (m_drums ? 2.0f : 2.5f);
				float texCoord = 0.0f;
				float tBeg = 0.0f, tEnd;
				for (Song::Beats::const_iterator it = m_song.beats.begin(); it != m_song.beats.end() && tBeg < future; ++it, texCoord += texCoordStep, tBeg = tEnd) {
					tEnd = *it - time;
					//if (tEnd < past) continue;
					if (tEnd > future) {
						// Crop the end off
						texCoord -= texCoordStep * (tEnd - future) / (tEnd - tBeg);
						tEnd = future;
					}
					glutil::Color c(1.0f, 1.0f, 1.0f, time2a(tEnd));
					glColor4fv(starpowerColorize(c, m_starpower.get()));
					glNormal3f(0.0f, 1.0f, 0.0f);
					glTexCoord2f(0.0f, texCoord); glVertex2f(-w, time2y(tEnd));
					glNormal3f(0.0f, 1.0f, 0.0f);
					glTexCoord2f(1.0f, texCoord); glVertex2f(w, time2y(tEnd));
				}
			}
			
			// Draw the cursor
			float level = m_hit[0].get();
			glColor3f(level, level, level);
			drawBar(0.0, 0.01f);
			// Fret buttons on cursor
			for (int fret = m_drums; fret < 5; ++fret) {
				float x = -2.0f + fret - 0.5f * m_drums;
				float l = m_hit[fret + !m_drums].get();
				// Get a color for the fret and adjust it if GodMode is on
				glColor4fv(starpowerColorize(color(fret), m_starpower.get()));
				m_button.dimensions.center(time2y(0.0)).middle(x);
				m_button.draw();
				glColor3f(l, l, l);
				m_tap.dimensions = m_button.dimensions;
				m_tap.draw();
			}
			
			// Draw the notes
			{ glutil::UseLighting lighting(m_use3d);
				for (Chords::iterator it = m_chords.begin(); it != m_chords.end(); ++it) {
					float tBeg = it->begin - time;
					float tEnd = it->end - time;
					if (tBeg > future) break;
					if (tEnd < past) {
						if (it->status < 100) it->status += 100; // Mark as past note for rewinding
						continue;
					}
					if (it->status >= 100 || (tBeg > maxTolerance
					  && it->status > 0)) continue; // Don't show past chords when rewinding
					for (int fret = 0; fret < 5; ++fret) { // Loop through the frets
						if (!it->fret[fret] || (tBeg > maxTolerance && it->releaseTimes[fret] > 0)) continue;
						if (tEnd > future) tEnd = future;
						unsigned event = m_notes[it->dur[fret]];
						float glow = 0.0f;
						float whammy = 0.0f;
						if (event > 0) {
							glow = m_events[event - 1].glow.get();
							whammy = m_events[event - 1].whammy.get();
						}
						// Get a color for the fret and adjust it if GodMode is on
						glutil::Color c = starpowerColorize(color(fret), m_starpower.get());
						if (glow > 0.1f) { ng_r+=c.r; ng_g+=c.g; ng_b+=c.b; ng_ccnt++; } // neck glow
						// Further adjust the color if the note is hit
						c.r += glow;
						c.g += glow;
						c.b += glow;
						if (glow > 0.5f && tEnd < 0.1f && it->hitAnim[fret].get() == 0.0)
							it->hitAnim[fret].setTarget(1.0);
						// Call the actual note drawing function
						drawNote(fret, c, tBeg, tEnd, whammy, it->tappable, glow > 0.5f, it->hitAnim[fret].get(), 
						  it->releaseTimes[fret] > 0.0 ? it->releaseTimes[fret] - time : 0.0);
					}
				}
			} //< disable lighting
			
			// Draw flames
			for (int fret = 0; fret < 5; ++fret) { // Loop through the frets
				if (m_drums && fret == 0) { // Skip bass drum
					m_flames[fret].clear(); continue;
				}
				Texture* ftex = &m_flame;
				if (m_starpower.get() > 0.01) ftex = &m_flame_godmode;
				float x = -2.0f + fret - 0.5f * m_drums;
				for (std::vector<AnimValue>::iterator it = m_flames[fret].begin(); it != m_flames[fret].end();) {
					float flameAnim = it->get();
					if (flameAnim < 1.0f) {
						float h = flameAnim * 4.0f * fretWid;
						UseTexture tblock(*ftex);
						glutil::Begin block(GL_TRIANGLE_STRIP);
						glColor4f(1.0f,1.0f,1.0f,1.0f);
						glTexCoord2f(0.0f, 1.0f); glVertex3f(x - fretWid, time2y(0.0f), 0.0f);
						glTexCoord2f(1.0f, 1.0f); glVertex3f(x + fretWid, time2y(0.0f), 0.0f);
						glTexCoord2f(0.0f, 0.0f); glVertex3f(x - fretWid, time2y(0.0f), h);
						glTexCoord2f(1.0f, 0.0f); glVertex3f(x + fretWid, time2y(0.0f), h);
					} else {
						it = m_flames[fret].erase(it);
						continue;
					}
					++it;
				}
			}
		} //< reverse scale sc1
		} //< reverse rot rot1
		} //< reverse trans tr2
		} //< reverse push pmb
		} //< reverse trans tr1
	} //< reverse push pmm
	
	// Bottom neck glow
	if (ng_ccnt > 0) {
		// Mangle color
		if (m_neckglowColor.r > 0 || m_neckglowColor.g > 0 || m_neckglowColor.b > 0) {
			m_neckglowColor.r = blend(m_neckglowColor.r, ng_r / ng_ccnt, 0.95);
			m_neckglowColor.g = blend(m_neckglowColor.g, ng_g / ng_ccnt, 0.95);
			m_neckglowColor.b = blend(m_neckglowColor.b, ng_b / ng_ccnt, 0.95);
		} else { // We don't want to fade from black in the start
			m_neckglowColor.r = ng_r / ng_ccnt;
			m_neckglowColor.g = ng_g / ng_ccnt;
			m_neckglowColor.b = ng_b / ng_ccnt;
		}
		m_neckglowColor.a = correctness();
	}
	if (correctness() > 0) {
		// Glow drawing
		glColor4fv(m_neckglowColor);
		m_neckglow.dimensions.screenBottom(0.0).middle(offsetX).fixedWidth(m_width.get());
		m_neckglow.draw();
	}
	drawInfo(time, offsetX, dimensions); // Go draw some texts and other interface stuff
	glColor3f(1.0f, 1.0f, 1.0f);
}

/// Draws a single note
/// The times passed are normalized to [past, future]
void GuitarGraph::drawNote(int fret, glutil::Color c, float tBeg, float tEnd, float whammy, bool tappable, bool hit, double hitAnim, double releaseTime) {
	float x = -2.0f + fret;
	if (m_drums) x -= 0.5f;
	if (m_drums && fret == 0) { // Bass drum? That's easy
		if (hit || hitAnim > 0) return;	// Hide it if it's hit
		c.a = time2a(tBeg); glColor4fv(c);
		drawBar(tBeg, 0.015f);
		return;
	}
	// If the note is hit, limit it to cursor position
	float yBeg = (hit || hitAnim > 0) ? std::min(time2y(0.0), time2y(tBeg)): time2y(tBeg);
	float yEnd = time2y(tEnd);
	// Long notes
	if (yBeg - 2 * fretWid >= yEnd) {
		// A hold is released? Let it go...
		if (releaseTime != 0.0 && tEnd - releaseTime > 0.1) yBeg = time2y(releaseTime);
		// Short note? Render minimum renderable length
		if (yEnd > yBeg - 3 * fretWid) yEnd = yBeg - 3 * fretWid;
		// Render the ring
		float y = yBeg + fretWid;
		if (m_use3d) { // 3D
			y -= fretWid;
			c.a = clamp(time2a(tBeg)*2.0f,0.0f,1.0f);
			glColor4fv(c);
			m_fretObj.draw(x, y, 0.0f);
			y -= fretWid;
		} else { // 2D
			UseTexture tblock(m_button_l);
			glutil::Begin block(GL_TRIANGLE_STRIP);
			c.a = time2a(tBeg); glColor4fv(c);
			vertexPair(x, y, c, 1.0f);
			y -= 2 * fretWid;
			vertexPair(x, y, c, 0.5f);
		}
		// Render the middle
		UseTexture tblock(m_button_l);
		glutil::Begin block(GL_TRIANGLE_STRIP);
		vertexPair(x, y, c, 0.5f);
		if (whammy > 0.1) {
			// Whammy uses more vertices
			while ((y -= fretWid) > yEnd + fretWid) {
				float r = rand() / double(RAND_MAX);
				vertexPair(x+cos(y*whammy)/4.0+(r-0.5)/4.0, y, c, 0.5f);
			}
		} else {
			while ((y -= 10.0) > yEnd + fretWid) vertexPair(x, y, c, 0.5f);
		}
		// Render the end
		y = yEnd + fretWid;
		vertexPair(x, y, c, 0.25f);
		vertexPair(x, yEnd, c, 0.0f);
	} else {
		// Too short note: only render the ring
		if (m_use3d) { // 3D
			if (hitAnim > 0.0 && tEnd <= maxTolerance) {
				float s = 1.0 - hitAnim;
				c.a = s; glColor4fv(c);
				m_fretObj.draw(x, yBeg, 0.0f, s);
			} else {
				c.a = clamp(time2a(tBeg)*2.0f,0.0f,1.0f); glColor4fv(c);
				m_fretObj.draw(x, yBeg, 0.0f);
			}
		} else { // 2D
			c.a = time2a(tBeg); glColor4fv(c);
			m_button.dimensions.center(yBeg).middle(x);
			m_button.draw();
		}
	}
	// Hammer note caps
	if (tappable) {
		float l = std::max(0.3, m_correctness.get());
		if (m_use3d) { // 3D
			float s = 1.0 - hitAnim;
			glColor4f(l, l, l, s);
			m_tappableObj.draw(x, yBeg, 0.0f, s);
		} else { // 2D
			glColor3f(l, l, l);
			m_tap.dimensions.center(yBeg).middle(x);
			m_tap.draw();
		}
	}
}

/// Draw popups and other info texts
void GuitarGraph::drawInfo(double time, double offsetX, Dimensions dimensions) {
	// Draw info
	if (time < m_jointime) {
		m_text.dimensions.screenBottom(-0.041).middle(-0.09 + offsetX);
		m_text.draw(diffv[m_level].name);
		m_text.dimensions.screenBottom(-0.015).middle(-0.09 + offsetX);
		m_text.draw(m_track_index->first);
	} else { // Draw scores
		m_text.dimensions.screenBottom(-0.30).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(getScore())));
		m_text.dimensions.screenBottom(-0.27).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(m_streak)) + "/" 
		  + boost::lexical_cast<std::string>(unsigned(m_longestStreak)));
	}
	// Is Starpower ready?
	if (canActivateStarpower()) {
		float a = (int(time * 1000.0) % 1000) / 1000.0;
		m_text.dimensions.screenBottom(-0.02).middle(-0.12 + offsetX);
		m_text.draw("God Mode Ready!", a);
	}
	// Draw streak pop-up for long streak intervals
	double streakAnim = m_streakPopup.get();
	if (streakAnim > 0.0) {
		double s = 0.2 * (1.0 + streakAnim);
		glColor4f(1.0f, 0.0f, 0.0f, 1.0 - streakAnim);
		m_popupText->render(boost::lexical_cast<std::string>(unsigned(m_bigStreak)) + "\nStreak!");
		m_popupText->dimensions().center(0.1).middle(offsetX).stretch(s,s);
		m_popupText->draw();
		if (streakAnim > 0.999) m_streakPopup.setTarget(0.0, true);
	}
	// Draw godmode activation pop-up
	double godAnim = m_godmodePopup.get();
	if (godAnim > 0.0) {
		float a = 1.0 - godAnim;
		float s = 0.2 * (1.0 + godAnim);
		glColor4f(0.3f, 0.0f, 1.0f, a);
		m_popupText->render("God Mode\nActivated!");
		m_popupText->dimensions().center(0.1).middle(offsetX).stretch(s,s);
		m_popupText->draw();
		m_text.dimensions.screenBottom(-0.02).middle(-0.12 + offsetX);
		m_text.draw("Mistakes ignored!", 1.0 - godAnim);
		if (godAnim > 0.999) m_godmodePopup.setTarget(0.0, true);
	}
}

/// Draw a bar for drum bass pedal/note
void GuitarGraph::drawBar(double time, float h) {
	glutil::Begin block(GL_TRIANGLE_STRIP);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glVertex2f(-2.5f, time2y(time + h));
	glVertex2f(2.5f, time2y(time + h));
	glVertex2f(-2.5f, time2y(time - h));
	glVertex2f(2.5f, time2y(time - h));
}

/// Create the Chord structures for the current track/difficulty level
void GuitarGraph::updateChords() {
	m_chords.clear();
	m_scoreFactor = 0;
	Durations::size_type pos[5] = {}, size[5] = {};
	Durations const* durations[5] = {};
	for (int fret = 0; fret < 5; ++fret) {
		NoteMap const& nm = m_track_index->second->nm;
		int basepitch = diffv[m_level].basepitch;
		NoteMap::const_iterator it = nm.find(basepitch + fret);
		if (it == nm.end()) continue;
		durations[fret] = &it->second;
		size[fret] = durations[fret]->size();
	}
	double lastEnd = 0.0;
	const double tapMaxDelay = 0.15;  // Delay from the end of the previous note
	while (true) {
		// Find the earliest
		double t = getInf();
		for (int fret = 0; fret < 5; ++fret) {
			if (pos[fret] == size[fret]) continue;
			Durations const& dur = *durations[fret];
			t = std::min(t, dur[pos[fret]].begin);
		}
		// Quit processing if none were left
		if (t == getInf()) break;
		// Construct a chord
		Chord c;
		c.begin = t;
		int tapfret = -1;
		for (int fret = 0; fret < 5; ++fret) {
			if (pos[fret] == size[fret]) continue;
			Durations const& dur = *durations[fret];
			Duration const& d = dur[pos[fret]];
			if (d.begin > t) continue;
			c.end = std::max(c.end, d.end);
			c.fret[fret] = true;
			c.dur[fret] = &d;
			tapfret = fret;
			++c.polyphony;
			++pos[fret];
			m_scoreFactor += 50;
			if (d.end - d.begin > 0.0) m_scoreFactor += 50.0 * (d.end - d.begin);
		}
		// Check if the chord is tappable
		if (!m_drums && c.polyphony == 1) {
			c.tappable = true;
			if (m_chords.empty() || m_chords.back().fret[tapfret]) c.tappable = false;
			if (lastEnd + tapMaxDelay < t) c.tappable = false;
		}
		lastEnd = c.end;
		m_chords.push_back(c);
	}
	m_chordIt = m_chords.begin();
	m_scoreFactor = 10000.0 / m_scoreFactor; // normalize maximum score factor
}

