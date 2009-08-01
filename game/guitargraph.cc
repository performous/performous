#include "guitargraph.hh"

namespace {
	struct Diff { std::string name; int basepitch; } diffv[] = {
		{ "Supaeasy", 0x3C },
		{ "Easy", 0x48 },
		{ "Medium", 0x54 },
		{ "Amazing", 0x60 }
	};
	const size_t diffsz = sizeof(diffv) / sizeof(*diffv);

	const float past = -0.3f;
	const float future = 3.0f;
	const float timescale = 60.0f;
	const float texCoordStep = -0.5f; // Two beat lines per neck texture => 0.5 tex units per beat
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return -timescale * (t - past) / (future - past); }
	float time2a(float t) {
		float a = clamp(1.0 - t / future); // Note: we want 1.0 alpha already at zero t.
		return std::pow(a, 0.8); // Nicer curve
	}

	double points(double error) {
		double score = 15;
		if (error < 0.1) score += 15;
		if (error < 0.05) score += 15;
		if (error < 0.03) score += 5;
		return score;
	}

}

GuitarGraph::GuitarGraph(Song const& song, bool drums):
  m_input(drums ? input::DRUMS : input::GUITAR),
  m_song(song),
  m_button("button.svg"),
  m_tap("tap.svg"),
  m_pickValue(0.0, 5.0),
  m_drums(drums),
  m_instrument(),
  m_level(),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_hammerReady(0.0, 2.0),
  m_score()
{
	std::size_t tracks = m_song.tracks.size();
	if (tracks == 0) throw std::runtime_error("No tracks");
	m_necks.push_back(new Texture("guitarneck.svg"));
	if (tracks > 1) m_necks.push_back(new Texture("bassneck.svg"));
	if (tracks > 2) m_necks.push_back(new Texture("drumneck.svg"));
	difficultyAuto();
}

void GuitarGraph::engine(double time) {
	for (input::Event ev; m_input.tryPoll(ev);) {
		if (m_drums) {
			if (ev.type == input::Event::PRESS) drumHit(time, ev.button);
		} else {
			if (time < -0.5) {
				if (ev.type == input::Event::PICK) {
					if (ev.pressed[4]) {
						++m_instrument;
						if (!difficulty(m_level)) difficultyAuto();
					}
					if (ev.pressed[0]) difficulty(DIFFICULTY_SUPAEASY);
					else if (ev.pressed[1]) difficulty(DIFFICULTY_EASY);
					else if (ev.pressed[2]) difficulty(DIFFICULTY_MEDIUM);
					else if (ev.pressed[3]) difficulty(DIFFICULTY_AMAZING);
				}
				continue;
			}
		}
		if (m_score < 0) m_score = 0;
	}
}

void GuitarGraph::drumHit(double time, int fret) {
	if (fret == 0) m_pickValue.setValue(1.0);
	int basepitch = diffv[m_level].basepitch;
	NoteMap const& nm = m_song.tracks[m_instrument].nm;
	NoteMap::const_iterator it = nm.find(basepitch + fret);
	if (it == nm.end()) return;
	Durations const& dur = it->second;
	Durations::const_iterator it2 = dur.begin(), it2tmp, it2end = dur.end();
	double tolerance = 0.2;
	// Find the first suitable note within the tolerance
	while (it2 != it2end && (it2->begin < time - tolerance || m_notes[&*it2] != 0)) ++it2;
	// If we are already past the accepted region, skip further processing
	if (it2 == it2end || it2->begin > time + tolerance) { m_score -= 100; return; }
	// Find the note with the smallest error
	it2tmp = it2;
	Duration const* d = NULL;
	do {
		it2 = it2tmp;
		if (m_notes[&*it2] != 0) return; // Notes already played are ignored
		d = &*it2;
		tolerance = std::abs(it2->begin - time);
	} while (++it2tmp != it2end && std::abs(it2tmp->begin - time) < tolerance);
	if (!d) throw std::logic_error("d is NULL in GuitarGraph::engine");
	m_score += points(tolerance);
	m_notes[d] = 2;
}

void GuitarGraph::guitarPlay(double time, input::Event const& ev) {
	bool picked = (ev.type == input::Event::PICK);
	bool const* frets = ev.pressed; // Kinda b0rked, FIXME
	if (picked) {
		m_pickValue.setValue(1.0);
	} else {
		if (m_hammerReady.get() < 0.5) return; // Hammering not possible at the moment
	}
	m_hammerReady.setTarget(0.0);
	// Find any suitable note within the tolerance
	double tolerance = 0.2;
	if (!picked) tolerance *= 0.5; // Hammering and drums have stricter limits
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
	std::cout << (picked ? "Hard: " : "Soft: ");
	if (best == m_chords.end()) {
		std::cout << "MISS" << std::endl;
		if (picked) {
			m_score -= 100;
			m_hammerReady.setTarget(0.0, true); // Instantly go to zero
		}
	} else {
		m_chordIt = best;
		int& score = m_chordIt->score;
		std::cout << "HIT, old score = " << score;
		m_score -= score;
		score = points(tolerance);
		score *= m_chordIt->polyphony;
		std::cout << ", new score = " << score << std::endl;
		m_chordIt->status = 1 + picked;
		m_score += score;
		m_hammerReady.setTarget(1.0, true); // Instantly go to one
	}
	if (m_score < 0) m_score = 0;
}

void GuitarGraph::difficultyAuto() {
	for (int level = 0; level < DIFFICULTYCOUNT; ++level) if (difficulty(Difficulty(level))) return;
	throw std::runtime_error("No difficulty levels found");
}

bool GuitarGraph::difficulty(Difficulty level) {
	m_instrument %= m_song.tracks.size();
	Track const& track = m_song.tracks[m_instrument];
	m_drums = (m_song.tracks[m_instrument].name == "DRUMS");
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
		glutil::Color(0.0f, 1.0f, 0.0f),
		glutil::Color(1.0f, 0.0f, 0.0f),
		glutil::Color(1.0f, 1.0f, 0.0f),
		glutil::Color(0.0f, 0.0f, 1.0f),
		glutil::Color(1.0f, 0.5f, 0.0f)
	};
	if (fret < 0 || fret > 4) throw std::logic_error("Invalid fret number in GuitarGraph::getColor");
	if (m_drums) {
		if (fret == 0) fret = 4;
		else if (fret == 4) fret = 0;
	}
	return fretColors[fret];
}

void GuitarGraph::draw(double time) {
	if (time < -0.5) {
		std::string txt = (m_drums ? "Hit a pad to change:\n" : "Play a fret to change:\n");
		txt += m_song.tracks[m_instrument].name + "/" + diffv[m_level].name;
		m_text.dimensions.screenBottom(-0.05).middle(0.0);
		m_text.draw(txt);
	} else {
		m_text.dimensions.screenBottom(-0.1).middle(0.2);
		m_text.draw(boost::lexical_cast<std::string>(m_score));
	}
	engine(time);
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenBottom().fixedWidth(0.5f);
	glutil::PushMatrix pmb;
	glTranslatef(0.5 * (dimensions.x1() + dimensions.x2()), dimensions.y2(), 0.0f);
	glRotatef(85.0f, 1.0f, 0.0f, 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }
	// Draw the neck
	{
		UseTexture tex(m_necks[m_instrument]);
		glutil::Begin block(GL_TRIANGLE_STRIP);
		float w = m_instrument == 2 ? 2.0f : 2.5f;
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
			float x = -2.0f + fret;
			if (m_drums) x -= 0.5f;
			float w = 0.5f;
			glutil::Color c = color(fret);
			if (m_notes[it->dur[fret]] == 1) c.r = c.g = c.b = 1.0f;
			float wLine = 0.5f * w;
			if (tEnd > future) tEnd = future;
			if (m_drums && fret == 0) {
				c.a = time2a(tBeg); glColor4fv(c);
				drawBar(tBeg, 0.01f);
				continue;
			}
			{
				glutil::Begin block(GL_TRIANGLE_STRIP);
				c.a = time2a(tEnd); glColor4fv(c);
				glVertex2f(x - wLine, time2y(tEnd));
				glVertex2f(x + wLine, time2y(tEnd));
				c.a = time2a(tBeg); glColor4fv(c);
				glVertex2f(x - wLine, time2y(tBeg));
				glVertex2f(x + wLine, time2y(tBeg));
			}
			m_button.dimensions.center(time2y(tBeg)).middle(x);
			m_button.draw();
			if (it->tappable) {
				float l = std::max(0.5, m_hammerReady.get());
				glColor3f(l, l, l);
				m_tap.dimensions = m_button.dimensions;
				m_tap.draw();
			}
		}
	}
	// Draw the cursor / bass pedal
	float level = m_pickValue.get();
	glColor3f(level, level, level);
	drawBar(0.0, 0.01f);
	// Fret buttons on cursor
	for (int fret = 0; fret < 5; ++fret) {
		float x = -2.0f + fret;
		if (m_drums) {
			if (fret == 0) continue;
			x -= 0.5f;
		}
		glutil::Color c = color(fret);
		if (m_input.pressed(fret)) {
			c.r = 1.0f;
			c.g = 1.0f;
			c.b = 1.0f;
		}
		glColor4fv(c);
		m_button.dimensions.center(time2y(0.0)).middle(x);
		m_button.draw();
	}
	glColor3f(1.0f, 1.0f, 1.0f);
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
	Durations::size_type pos[5] = {}, size[5] = {};
	Durations const* durations[5] = {};
	for (int fret = 0; fret < 5; ++fret) {
		NoteMap const& nm = m_song.tracks[m_instrument].nm;
		int basepitch = diffv[m_level].basepitch;
		NoteMap::const_iterator it = nm.find(basepitch + fret);
		if (it == nm.end()) continue;
		durations[fret] = &it->second;
		size[fret] = durations[fret]->size();
	}
	double lastEnd = 0.0;
	const double tapMaxDelay = 0.05;  // Delay from the end of the previous note
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
		for (int fret = 0; fret < 5; ++fret) {
			if (pos[fret] == size[fret]) continue;
			Durations const& dur = *durations[fret];
			Duration const& d = dur[pos[fret]];
			if (d.begin > t) continue;
			c.end = std::max(c.end, d.end);
			c.fret[fret] = true;
			c.dur[fret] = &d;
			++c.polyphony;
			++pos[fret];
		}
		if (c.polyphony == 1) {
			c.tappable = true;
			if (m_chords.empty() || m_chords.back() == c) c.tappable = false;
			if (lastEnd + tapMaxDelay < t) c.tappable = false;
		}
		lastEnd = c.end;
		m_chords.push_back(c);
	}
	m_chordIt = m_chords.begin();
}

