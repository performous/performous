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

	const double maxTolerance = 0.15;
	
	double points(double error) {
		double score = 0.0;
		if (error < maxTolerance) score += 15;
		if (error < 0.1) score += 15;
		if (error < 0.05) score += 15;
		if (error < 0.03) score += 5;
		return score;
	}

}

GuitarGraph::GuitarGraph(Song const& song, bool drums, unsigned track):
  m_input(drums ? input::DRUMS : input::GUITAR),
  m_song(song),
  m_button("button.svg"),
  m_tap("tap.svg"),
  m_drums(drums),
  m_cx(0.0, 0.2),
  m_width(0.5, 0.4),
  m_track(track),
  m_stream(),
  m_level(),
  m_dead(1000),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_correctness(0.0, 5.0),
  m_score()
{
	for (Tracks::const_iterator it = m_song.tracks.begin(); it != m_song.tracks.end(); ++it) {
		if (drums != (it->name == "DRUMS")) continue;
		m_tracks.push_back(&*it);
		if (it->name == "DRUMS") m_necks.push_back(new Texture("drumneck.svg"));
		else if (it->name == "BASS") m_necks.push_back(new Texture("bassneck.svg"));
		else m_necks.push_back(new Texture("guitarneck.svg"));
	}
	for (int i = 0; i < 6; ++i) m_hit[i].setRate(5.0);
	if (m_tracks.empty()) throw std::runtime_error("No tracks");
	difficultyAuto();
}

void GuitarGraph::engine(Audio& audio) {
	double time = audio.getPosition();
	time -= config["audio/controller_delay"].f();
	// Handle holds
	if (!m_drums) {
		for (int i = 0; i < 5; ++i) {
			if (m_input.pressed(i)) {
				m_hit[i + 1].setValue(1.0);
				// TODO: score holds etc
			} else {
				// TODO: Release holds
			}
		
		}
	}
	// Handle all events
	for (input::Event ev; m_input.tryPoll(ev);) {
		m_dead = false;
		if (ev.type == input::Event::PRESS) m_hit[!m_drums + ev.button].setValue(1.0);
		else if (ev.type == input::Event::PICK) m_hit[0].setValue(1.0);
		if (time < -0.5) {
			if (ev.type == input::Event::PICK || (m_drums && ev.type == input::Event::PRESS)) {
				if (ev.pressed[4]) {
					++m_track;
					if (!difficulty(m_level)) difficultyAuto();
				}
				if (ev.pressed[0]) difficulty(DIFFICULTY_SUPAEASY);
				else if (ev.pressed[1]) difficulty(DIFFICULTY_EASY);
				else if (ev.pressed[2]) difficulty(DIFFICULTY_MEDIUM);
				else if (ev.pressed[3]) difficulty(DIFFICULTY_AMAZING);
			}
		} else if (m_drums) {
			if (ev.type == input::Event::PRESS) drumHit(time, ev.button);
		} else {
			if (ev.type == input::Event::PRESS || ev.type == input::Event::PICK) guitarPlay(time, ev);
		}
		if (m_score < 0) m_score = 0;
	}
	// Skip missed notes
	while (m_chordIt != m_chords.end() && m_chordIt->begin + maxTolerance < time) {
		if (m_chordIt->status < m_chordIt->polyphony) m_correctness.setTarget(0.0);
		++m_chordIt;
		++m_dead;
	}
	if (m_chordIt != m_chords.end() && m_chordIt->end < time) m_correctness.setTarget(1.0);
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
	if (best == m_chords.end()) {
		std::cout << "MISS" << std::endl;
		m_score -= 50;
		m_correctness.setTarget(0.0, true); // Instantly go to zero
	} else {
		while (best != m_chordIt) {
			if (m_chordIt->status == m_chordIt->polyphony) continue;
			m_correctness.setTarget(0.0);
			std::cout << "SKIPPED, ";
			++m_chordIt;
		}
		++m_chordIt->status;
		m_events.push_back(Event(time, 1));
		m_notes[m_chordIt->dur[fret]] = m_events.size();
		double score = points(tolerance);
		m_chordIt->score += score;
		m_score += score;
		if (m_chordIt->status == m_chordIt->polyphony) {
			m_score -= m_chordIt->score;
			m_chordIt->score *= m_chordIt->polyphony;
			m_score += m_chordIt->score;
			std::cout << "FULL HIT!";
		} else {
			std::cout << "HIT";
		}
	}
	std::cout << std::endl;
}

void GuitarGraph::guitarPlay(double time, input::Event const& ev) {
	bool picked = (ev.type == input::Event::PICK);
	bool frets[5] = {};
	if (picked) {
		for (int fret = 0; fret < 5; ++fret) frets[fret] = ev.pressed[fret];
	} else {
		if (m_correctness.get() < 0.5) return; // Hammering not possible at the moment
		frets[ev.button] = true;
		for (int fret = ev.button + 1; fret < 5; ++fret) if (ev.pressed[fret]) return; // Extra buttons on right side
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
	if (best == m_chords.end()) {
		std::cout << "MISS" << std::endl;
		if (!picked) return;
		m_score -= 50;
		m_correctness.setTarget(0.0, true); // Instantly go to zero
		m_events.push_back(Event(time, 0));
	} else {
		m_chordIt = best;
		int& score = m_chordIt->score;
		std::cout << "HIT, error = " << int(1000.0 * (best->begin - time)) << " ms" << std::endl;
		m_score -= score;
		score = points(tolerance);
		score *= m_chordIt->polyphony;
		m_chordIt->status = 1 + picked;
		m_score += score;
		m_correctness.setTarget(1.0, true); // Instantly go to one
		m_events.push_back(Event(time, 1 + picked));
		for (int fret = 0; fret < 5; ++fret) {
			if (m_chordIt->fret[fret]) m_notes[m_chordIt->dur[fret]] = m_events.size();
		}
	}
}

void GuitarGraph::difficultyAuto() {
	for (int level = 0; level < DIFFICULTYCOUNT; ++level) if (difficulty(Difficulty(level))) return;
	throw std::runtime_error("No difficulty levels found");
}

bool GuitarGraph::difficulty(Difficulty level) {
	m_track %= m_tracks.size();
	Track const& track = *m_tracks[m_track];
	// Find the stream number
	for (m_stream = 0; m_stream < m_song.tracks.size(); ++m_stream) {
		if (&track == &m_song.tracks[m_stream]) break;
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
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenBottom().middle(m_cx.get()).fixedWidth(m_width.get());
	double offsetX = 0.5 * (dimensions.x1() + dimensions.x2());
	double frac = 0.75;  // Adjustable: 1.0 means fully separated, 0.0 means fully attached
	if (time < -0.5) {
		std::string txt;
		txt += m_tracks[m_track]->name + "\n" + diffv[m_level].name;
		m_text.dimensions.screenBottom(-0.05).middle(-0.1 + offsetX);
		m_text.draw(txt);
	} else {
		m_text.dimensions.screenBottom(-0.15).middle(0.4 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(m_score));
	}
	glutil::PushMatrixMode pmm(GL_PROJECTION);
	glTranslatef(frac * 2.0 * offsetX, 0.0f, 0.0f);
	glutil::PushMatrixMode pmb(GL_MODELVIEW);
	glTranslatef((1.0 - frac) * offsetX, dimensions.y2(), 0.0f);
	glRotatef(85.0f, 1.0f, 0.0f, 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }
	// Draw the neck
	{
		UseTexture tex(m_necks[m_track]);
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
			float x = -2.0f + fret;
			if (m_drums) x -= 0.5f;
			float w = 0.5f;
			glutil::Color c = color(fret);
			unsigned event = m_notes[it->dur[fret]];
			if (event) {
				c.r = c.g = c.b = std::max(0.5, m_events[event - 1].glow.get());
			}
			float wLine = 0.5f * w;
			if (tEnd > future) tEnd = future;
			if (m_drums && fret == 0) {
				c.a = time2a(tBeg); glColor4fv(c);
				drawBar(tBeg, 0.01f);
				continue;
			}
			if (tEnd > tBeg) {
				glutil::Begin block(GL_TRIANGLE_STRIP);
				for (float t = tEnd; true; t -= 0.1f) {
					if (t < tBeg) t = tBeg;
					c.a = time2a(t); glColor4fv(c);
					glVertex2f(x - wLine, time2y(t));
					glVertex2f(x + wLine, time2y(t));
					if (t == tBeg) break;
				}
			}
			c.a = time2a(tBeg); glColor4fv(c);
			m_button.dimensions.center(time2y(tBeg)).middle(x);
			m_button.draw();
			if (it->tappable) {
				float l = std::max(0.5, m_correctness.get());
				glColor3f(l, l, l);
				m_tap.dimensions = m_button.dimensions;
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
		NoteMap const& nm = m_tracks[m_track]->nm;
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
		}
		if (c.polyphony == 1) {
			c.tappable = true;
			if (m_chords.empty() || m_chords.back().fret[tapfret]) c.tappable = false;
			if (lastEnd + tapMaxDelay < t) c.tappable = false;
		}
		lastEnd = c.end;
		m_chords.push_back(c);
	}
	m_chordIt = m_chords.begin();
}

