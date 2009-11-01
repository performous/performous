#include "guitargraph.hh"

#include "fs.hh"
#include "song.hh"
#include <cmath>
#include <cstdlib>

#include <boost/lexical_cast.hpp>

namespace {
	struct Diff { std::string name; int basepitch; } diffv[] = {
		{ "Supaeasy", 0x3C },
		{ "Easy", 0x48 },
		{ "Medium", 0x54 },
		{ "Amazing", 0x60 }
	};
	const size_t diffsz = sizeof(diffv) / sizeof(*diffv);

	const float g_angle = 80.0f;
	const float past = -0.2f;
	const float future = 1.8f;
	const float timescale = 25.0f;
	const float texCoordStep = -0.5f; // Two beat lines per neck texture => 0.5 tex units per beat
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

	std::vector<Sample> g_samplesG, g_samplesD;

}

GuitarGraph::GuitarGraph(Audio& audio, Song const& song, std::string track):
  m_audio(audio),
  m_input(track=="drums" ? input::DRUMS : input::GUITAR),
  m_song(song),
  m_button("button.svg"),
  m_button_l("button_l.svg"),
  m_tap("tap.svg"),
  m_drums(track=="drums"),
  m_cx(0.0, 0.2),
  m_width(0.5, 0.4),
  m_stream(),
  m_track_index(m_track_map.end()),
  m_level(),
  m_dead(1000),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_correctness(0.0, 5.0),
  m_score(),
  m_scoreFactor(),
  m_streak(),
  m_longestStreak()
{
	unsigned int sr = m_audio.getSR();
	if (g_samplesD.empty()) {
		g_samplesD.push_back(Sample(getDataPath("sounds/drum_bass.ogg"), sr));
		g_samplesD.push_back(Sample(getDataPath("sounds/drum_snare.ogg"), sr));
		g_samplesD.push_back(Sample(getDataPath("sounds/drum_hi-hat.ogg"), sr));
		g_samplesD.push_back(Sample(getDataPath("sounds/drum_tom1.ogg"), sr));
		g_samplesD.push_back(Sample(getDataPath("sounds/drum_cymbal.ogg"), sr));
		//g_samplesD.push_back(Sample(getDataPath("sounds/drum_tom2.ogg"), sr));
		g_samplesG.push_back(Sample(getDataPath("sounds/guitar_fail1.ogg"), sr));
		g_samplesG.push_back(Sample(getDataPath("sounds/guitar_fail2.ogg"), sr));
		g_samplesG.push_back(Sample(getDataPath("sounds/guitar_fail3.ogg"), sr));
		g_samplesG.push_back(Sample(getDataPath("sounds/guitar_fail4.ogg"), sr));
		g_samplesG.push_back(Sample(getDataPath("sounds/guitar_fail5.ogg"), sr));
		g_samplesG.push_back(Sample(getDataPath("sounds/guitar_fail6.ogg"), sr));
	}
	unsigned int i = 0;
	for (TrackMap::const_iterator it = m_song.track_map.begin(); it != m_song.track_map.end(); ++it,++i) {
		std::string index = it->first;
		TrackMapConstPtr::const_iterator it2 = m_track_map.insert(std::make_pair(index, &it->second)).first;
		if (index == track) m_track_index = it2;
	}
	if (m_track_index == m_track_map.end()) throw std::runtime_error("Could not find track \"" + track + "\"");
	for (int i = 0; i < 6; ++i) m_hit[i].setRate(5.0);
	for (int i = 0; i < 5; ++i) m_holds[i] = 0;
	if (m_track_map.empty()) throw std::runtime_error("No track");
	difficultyAuto();
	updateNeck();
}

void GuitarGraph::updateNeck() {
	// TODO: Optimize with texture cache
	std::string index = m_track_index->first;
	if (index == "drums") m_neck.reset(new Texture("drumneck.svg"));
	else if (index == "bass") m_neck.reset(new Texture("bassneck.svg"));
	else m_neck.reset(new Texture("guitarneck.svg"));
}

void GuitarGraph::engine() {
	double time = m_audio.getPosition();
	time -= config["audio/controller_delay"].f();
	// Handle key markers
	if (!m_drums) {
		for (int i = 0; i < 5; ++i) {
			if (m_input.pressed(i)) m_hit[i + 1].setValue(1.0);
		}
	}
	double whammy = 0;
	// Handle all events
	for (input::Event ev; m_input.tryPoll(ev);) {
		m_dead = false;
		if (!m_drums && ev.type == input::Event::RELEASE) {
			endHold(ev.button);
		}
		if (!m_drums && ev.type == input::Event::WHAMMY) whammy = (1.0 + ev.button) / 2.0;
		if (ev.type == input::Event::PRESS) m_hit[!m_drums + ev.button].setValue(1.0);
		else if (ev.type == input::Event::PICK) m_hit[0].setValue(1.0);
		if (time < -0.5) {
			if (ev.type == input::Event::PICK || (m_drums && ev.type == input::Event::PRESS)) {
				if (!m_drums && ev.pressed[4]) nextTrack();
				if (ev.pressed[0 + m_drums]) difficulty(DIFFICULTY_SUPAEASY);
				else if (ev.pressed[1 + m_drums]) difficulty(DIFFICULTY_EASY);
				else if (ev.pressed[2 + m_drums]) difficulty(DIFFICULTY_MEDIUM);
				else if (ev.pressed[3 + m_drums]) difficulty(DIFFICULTY_AMAZING);
			}
		} else if (m_drums) {
			if (ev.type == input::Event::PRESS) drumHit(time, ev.button);
		} else {
			guitarPlay(time, ev);
		}
		if (m_score < 0) m_score = 0;
	}
	// Skip missed notes
	while (m_chordIt != m_chords.end() && m_chordIt->begin + maxTolerance < time) {
		++m_chordIt;
		++m_dead;
	}
	if (!m_events.empty() && m_events.back().type == 0) m_correctness.setTarget(0.0, true);
	else if (m_chordIt != m_chords.end() && m_chordIt->begin <= time) {
		double level;
		if (m_drums) level = double(m_chordIt->status) / m_chordIt->polyphony;
		else level = m_chordIt->status ? 1.0 : 0.0;
		m_correctness.setTarget(level);
	}
	if (m_correctness.get() == 0) m_streak = 0;
	if (!m_drums) {
		// Processing holds
		for (int fret = 0; fret < 5; ++fret) {
			if (!m_holds[fret]) continue;
			Event& ev = m_events[m_holds[fret] - 1];
			ev.glow.setTarget(1.0, true);
			ev.whammy.setTarget(whammy);
			if (whammy > 0) {
				ev.whammy.move(0.5);
				if (ev.whammy.get() > 1.0) ev.whammy.setValue(1.0);
			}
			double last = std::min(time, ev.dur->end);
			double t = last - ev.holdTime;
			if (t > 0) {
				m_score += t * 50.0;
				ev.holdTime = time;
			}
			if (last == ev.dur->end) endHold(fret);
		}
	}
}

void GuitarGraph::endHold(int fret) {
	if (!m_holds[fret]) return;
	m_events[m_holds[fret] - 1].glow.setTarget(0.0);
	m_events[m_holds[fret] - 1].whammy.setTarget(0.0, true);
	m_holds[fret] = 0;
}

void GuitarGraph::fail(double time, int fret) {
	std::cout << "MISS" << std::endl;
	if (fret == -2) return; // Tapped note
	m_events.push_back(Event(time, 0, fret));
	if (fret < 0) fret = std::rand();
	std::vector<Sample> const& samples = (m_drums ? g_samplesD : g_samplesG);
	m_audio.play(samples[unsigned(fret) % samples.size()], "audio/fail_volume");
	m_score -= 50;
	m_streak = 0;
}

void GuitarGraph::drumHit(double time, int fret) {
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
	std::cout << "Drum: ";
	if (best == m_chords.end()) fail(time, fret);
	else {
		for (; best != m_chordIt; ++m_chordIt) {
			if (m_chordIt->status == m_chordIt->polyphony) continue;
			std::cout << "SKIPPED, ";
			m_streak = 0;
		}
		++m_chordIt->status;
		Duration const* dur = m_chordIt->dur[fret];
		m_events.push_back(Event(time, 1, fret, dur));
		m_notes[dur] = m_events.size();
		double score = points(tolerance);
		m_chordIt->score += score;
		m_score += score;
		if (m_chordIt->status == m_chordIt->polyphony) {
			//m_score -= m_chordIt->score;
			//m_chordIt->score *= m_chordIt->polyphony;
			//m_score += m_chordIt->score;
			m_streak += 1;
			if (m_streak > m_longestStreak) m_longestStreak = m_streak;
			std::cout << "FULL HIT!" << std::endl;
		} else {
			std::cout << "HIT" << std::endl;
		}
		m_correctness.setTarget(double(m_chordIt->status) / m_chordIt->polyphony, true);
	}
}

void GuitarGraph::guitarPlay(double time, input::Event const& ev) {
	bool picked = (ev.type == input::Event::PICK);
	bool frets[5] = {};  // The combination about to be played
	if (picked) {
		for (int fret = 0; fret < 5; ++fret) {
			frets[fret] = ev.pressed[fret];
			endHold(fret);
		}
	} else {
		if (m_correctness.get() < 0.5) return; // Hammering not possible at the moment
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
			if (tmp == m_chords.begin() || (--tmp)->status == 0) continue; // The previous note not played
		}
		if (!it->matches(frets)) continue;
		double error = std::abs(it->begin - time);
		if (error < tolerance) {
			best = it;
			tolerance = error;
		}
	}
	std::cout << (picked ? "Pick: " : "Tap: ");
	if (best == m_chords.end()) fail(time, picked ? -1 : -2);
	else {
		m_chordIt = best;
		int& score = m_chordIt->score;
		std::cout << "HIT, error = " << int(1000.0 * (best->begin - time)) << " ms" << std::endl;
		m_score -= score;
		score = points(tolerance);
		score *= m_chordIt->polyphony;
		m_chordIt->status = 1 + picked;
		m_score += score;
		m_streak += 1;
		if (m_streak > m_longestStreak) m_longestStreak = m_streak;
		m_correctness.setTarget(1.0, true); // Instantly go to one
		for (int fret = 0; fret < 5; ++fret) {
			if (!m_chordIt->fret[fret]) continue;
			Duration const* dur = m_chordIt->dur[fret];
			m_events.push_back(Event(time, 1 + picked, fret, dur));
			m_notes[dur] = m_events.size();
			m_holds[fret] = m_events.size();
		}
	}
}

void GuitarGraph::nextTrack() {
	while (1) {
		if (++m_track_index == m_track_map.end()) m_track_index = m_track_map.begin();
		if (m_track_index->first != "drums") break;  // Only accept non-drum tracks
	}
	difficultyAuto();
	updateNeck();
}

void GuitarGraph::difficultyAuto(bool tryKeep) {
	if (tryKeep && difficulty(Difficulty(m_level))) return;
	for (int level = 0; level < DIFFICULTYCOUNT; ++level) if (difficulty(Difficulty(level))) return;
	throw std::runtime_error("No difficulty levels found");
}

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
	m_level = level;
	updateChords();
	return true;
}

glutil::Color const& GuitarGraph::color(int fret) const {
	static glutil::Color fretColors[5] = {
		glutil::Color(0.0f, 0.9f, 0.0f),
		glutil::Color(0.9f, 0.0f, 0.0f),
		glutil::Color(0.8f, 0.8f, 0.0f),
		glutil::Color(0.0f, 0.0f, 1.0f),
		glutil::Color(0.8f, 0.4f, 0.0f)
	};
	if (fret < 0 || fret > 4) throw std::logic_error("Invalid fret number in GuitarGraph::getColor");
	if (m_drums) {
		if (fret == 0) fret = 4;
		else if (fret == 4) fret = 0;
	}
	return fretColors[fret];
}

void GuitarGraph::draw(double time) {
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenBottom().middle(m_cx.get()).fixedWidth(m_width.get());
	double offsetX = 0.5 * (dimensions.x1() + dimensions.x2());
	double frac = 0.75;  // Adjustable: 1.0 means fully separated, 0.0 means fully attached
	// Draw scores
	if (time < -0.5) {
		std::string txt;
		txt += m_track_index->first + "\n" + diffv[m_level].name;
		m_text.dimensions.screenBottom(-0.05).middle(-0.1 + offsetX);
		m_text.draw(txt);
	} else {
		m_text.dimensions.screenBottom(-0.30).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(getScore())));
		m_text.dimensions.screenBottom(-0.27).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(m_streak)) + "/" 
		  + boost::lexical_cast<std::string>(unsigned(m_longestStreak)));
	}
	glutil::PushMatrixMode pmm(GL_PROJECTION);
	glTranslatef(frac * 2.0 * offsetX, 0.0f, 0.0f);
	glutil::PushMatrixMode pmb(GL_MODELVIEW);
	glTranslatef((1.0 - frac) * offsetX, dimensions.y2(), 0.0f);
	glRotatef(g_angle, 1.0f, 0.0f, 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }
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
			glColor4f(1.0f, 1.0f, 1.0f, time2a(tEnd));
			glTexCoord2f(0.0f, texCoord); glVertex2f(-w, time2y(tEnd));
			glTexCoord2f(1.0f, texCoord); glVertex2f(w, time2y(tEnd));
		}
	}
	// Draw the notes
	for (Chords::const_iterator it = m_chords.begin(); it != m_chords.end(); ++it) {
		float tBeg = it->begin - time;
		float tEnd = it->end - time;
		if (tEnd < past) continue;
		if (tBeg > future) break;
		for (int fret = 0; fret < 5; ++fret) {
			if (!it->fret[fret]) continue;
			if (tEnd > future) tEnd = future;
			//drawNote(fret, color(fret), tBeg, tEnd);
			unsigned event = m_notes[it->dur[fret]];
			float glow = 0.0f;
			float whammy = 0.0f;
			if (event > 0) {
				glow = m_events[event - 1].glow.get();
				whammy = m_events[event - 1].whammy.get();
			}
			/*
			if (glow > 0.0f) {
				glBlendFunc(GL_ONE, GL_ONE);
				drawNote(fret, glutil::Color(glow, glow, glow), tBeg, tEnd);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			*/
			glutil::Color c = color(fret);
			c.r += glow;
			c.g += glow;
			c.b += glow;
			drawNote(fret, c, tBeg, tEnd, whammy);

			if (it->tappable) {
				float l = std::max(0.5, m_correctness.get());
				glColor3f(l, l, l);
				float x = -2.0f + fret;
				m_tap.dimensions.center(time2y(tBeg)).middle(x);
				m_tap.draw();
			}
		}
	}
	// Draw the cursor
	float level = m_hit[0].get();
	glColor3f(level, level, level);
	drawBar(0.0, 0.01f);
	// Fret buttons on cursor
	for (int fret = m_drums; fret < 5; ++fret) {
		float x = -2.0f + fret - 0.5f * m_drums;
		glColor4fv(color(fret));
		m_button.dimensions.center(time2y(0.0)).middle(x);
		m_button.draw();
		float l = m_hit[fret + !m_drums].get();
		glColor3f(l, l, l);
		m_tap.dimensions = m_button.dimensions;
		m_tap.draw();
	}
	glColor3f(1.0f, 1.0f, 1.0f);
}

namespace {
	const float fretWid = 0.5f;
	void vertexPair(float x, float y, glutil::Color c, float ty) {
		c.a = y2a(y); glColor4fv(c);
		glTexCoord2f(0.0f, ty); glVertex2f(x - fretWid, y);
		glTexCoord2f(1.0f, ty); glVertex2f(x + fretWid, y);
	}
}

void GuitarGraph::drawNote(int fret, glutil::Color c, float tBeg, float tEnd, float whammy) {
	float x = -2.0f + fret;
	if (m_drums) x -= 0.5f;
	if (m_drums && fret == 0) {
		c.a = time2a(tBeg); glColor4fv(c);
		drawBar(tBeg, 0.01f);
		return;
	}
	float yBeg = time2y(tBeg);
	float yEnd = time2y(tEnd);
	if (yBeg - 2 * fretWid >= yEnd) {
		if (yEnd > yBeg - 3 * fretWid) yEnd = yBeg - 3 * fretWid;  // Short note: render minimum renderable length
		UseTexture tblock(m_button_l);
		glutil::Begin block(GL_TRIANGLE_STRIP);
		c.a = time2a(tBeg); glColor4fv(c);
		// Render the ring
		float y = yBeg + fretWid;
		vertexPair(x, y, c, 1.0f);
		y -= 2 * fretWid;
		vertexPair(x, y, c, 0.5f);
		// Render the middle
		if (whammy > 0.1) {
			while ((y -= fretWid) > yEnd + fretWid) {
				// The sin formula is pure magic
				vertexPair(x+sin((whammy-0.75)*6.0 + (yBeg-tEnd)/y)/3.0, y, c, 0.5f);
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
		c.a = time2a(tBeg); glColor4fv(c);
		m_button.dimensions.center(time2y(tBeg)).middle(x);
		m_button.draw();
	}
}

void GuitarGraph::drawBar(double time, float h) {
	glutil::Begin block(GL_TRIANGLE_STRIP);
	glVertex2f(-2.5f, time2y(time + h));
	glVertex2f(2.5f, time2y(time + h));
	glVertex2f(-2.5f, time2y(time - h));
	glVertex2f(2.5f, time2y(time - h));
}

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

