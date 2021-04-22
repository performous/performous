#include "notegraph.hh"

#include "configuration.hh"
#include "database.hh"
#include "engine.hh"
#include "player.hh"

Dimensions dimensions; // Make a public member variable

NoteGraph::NoteGraph(VocalTrack const& vocal):
  m_vocal(vocal),
  m_notelines(findFile("notelines.svg")), m_wave(findFile("wave.svg")),
  m_star(findFile("star.svg")), m_star_hl(findFile("star_glow.svg")),
  m_notebar(findFile("notebar.svg")), m_notebar_hl(findFile("notebar_hi.svg")),
  m_notebarfs(findFile("notebarfs.svg")), m_notebarfs_hl(findFile("notebarfs_hi.svg")),
  m_notebargold(findFile("notebargold.svg")), m_notebargold_hl(findFile("notebargold_hi.svg")),
  m_notealpha(0.0f), m_nlTop(0.0, 4.0), m_nlBottom(0.0, 4.0), m_time()
{
	dimensions.stretch(1.0, 0.5); // Initial dimensions, probably overridden from somewhere
	m_nlTop.setTarget(m_vocal.noteMax, true);
	m_nlBottom.setTarget(m_vocal.noteMin, true);
	for (auto const& n: m_vocal.notes) n.stars.clear(); // Reset stars
	reset();
}

void NoteGraph::reset() {
	m_songit = m_vocal.notes.begin();
}

namespace {
	void drawNotebar(Texture const& texture, double x, double ybeg, double yend, double w, double h_x, double h_y) {
		glutil::VertexArray va;
		UseTexture tblock(texture);

		// The front cap begins
		va.texCoord(0.0f, 0.0f).vertex(x, ybeg);
		va.texCoord(0.0f, 1.0f).vertex(x, ybeg + h_y);
		if (w >= 2.0 * h_x) {
			// Calculate the y coordinates of the middle part
			double tmp = h_x / w;  // h_x = cap size (because it is a h_x by h_x square)
			double y1 = (1.0 - tmp) * ybeg + tmp * yend;
			double y2 = tmp * ybeg + (1.0 - tmp) * yend;
			// The middle part between caps
			va.texCoord(0.5f, 0.0f).vertex(x + h_x, y1);
			va.texCoord(0.5f, 1.0f).vertex(x + h_x, y1 + h_y);
			va.texCoord(0.5f, 0.0f).vertex(x + w - h_x, y2);
			va.texCoord(0.5f, 1.0f).vertex(x + w - h_x, y2 + h_y);
		} else {
			// Note is too short to even fit caps, crop to fit.
			double ymid = 0.5 * (ybeg + yend);
			float crop = 0.25f * w / h_x;
			va.texCoord(crop, 0.0f).vertex(x + 0.5 * w, ymid);
			va.texCoord(crop, 1.0f).vertex(x + 0.5 * w, ymid + h_y);
			va.texCoord(1.0f - crop, 0.0f).vertex(x + 0.5 * w, ymid);
			va.texCoord(1.0f - crop, 1.0f).vertex(x + 0.5 * w, ymid + h_y);
		}
		// The rear cap ends
		va.texCoord(1.0f, 0.0f).vertex(x + w, yend);
		va.texCoord(1.0f, 1.0f).vertex(x + w, yend + h_y);

		va.draw();
	}
}

const double baseLine = -0.2;
const double pixUnit = 0.2;

void NoteGraph::draw(double time, Database const& database, Position position) {
	if (time < m_time) reset();
	m_time = time;
	// Update m_songit (which note to start the rendering from)
	while (m_songit != m_vocal.notes.end() && (m_songit->type == Note::SLEEP || m_songit->end < time - (baseLine + 0.5) / pixUnit)) ++m_songit;

	// Automatically zooming notelines
	{
		int low = m_vocal.noteMax;
		int high = m_vocal.noteMin;
		int low2 = m_vocal.noteMax;
		int high2 = m_vocal.noteMin;
		for (auto it = m_songit; it != m_vocal.notes.end() && it->begin < time + 15.0; ++it) {
			if (it->type == Note::SLEEP) continue;
			if (it->note < low) low = it->note;
			if (it->note > high) high = it->note;
			if (it->begin > time + 8.0) continue;
			if (it->note < low2) low2 = it->note;
			if (it->note > high2) high2 = it->note;
		}
		if (low2 <= high2) {
			m_nlTop.setRange(high2, high);
			m_nlBottom.setRange(low, low2);
		}
	}
	switch(position) {
		case NoteGraph::FULLSCREEN:
			dimensions.stretch(1.0, 0.50).center();
			break;
		case NoteGraph::TOP:
			dimensions.stretch(1.0, 0.32).bottom(0.0);
			break;
		case NoteGraph::BOTTOM:
			dimensions.stretch(1.0, 0.32).top(0.0);
			break;
		case NoteGraph::LEFT:
			dimensions.stretch(0.50, 0.50).center().left(-0.5);
			break;
		case NoteGraph::RIGHT:
			dimensions.stretch(0.50, 0.50).center().right();
			break;
	}
	m_max = m_nlTop.get() + 7.0;
	m_min = m_nlBottom.get() - 7.0;
	m_noteUnit = -dimensions.h() / std::max(48.0 * dimensions.h(), m_max - m_min);
	m_baseY = -0.5 * (m_min + m_max) * m_noteUnit + dimensions.yc();
	m_baseX = baseLine - m_time * pixUnit + dimensions.xc();  // FIXME: Moving in X direction requires additional love (is b0rked now, keep it centered at zero)

	// Fading notelines handing
	if (m_songit == m_vocal.notes.end() || m_songit->begin > m_time + 3.0) m_notealpha -= 0.02f;
	else if (m_notealpha < 1.0f) m_notealpha += 0.02f;
	if (m_notealpha <= 0.0f) { m_notealpha = 0.0f; return; }

	ColorTrans c(Color::alpha(m_notealpha));

	drawNotes();
	if (config["game/pitch"].b()) drawWaves(database);

	// Draw a star for well sung notes
	for (auto it = m_songit; it != m_vocal.notes.end() && it->begin < m_time - (baseLine - 0.5) / pixUnit; ++it) {
		float player_star_offset = 0;
		for (std::vector<Color>::const_iterator it_col = it->stars.begin(); it_col != it->stars.end(); ++it_col) {
			double x = m_baseX + it->begin * pixUnit + m_noteUnit; // left x coordinate: begin minus border (side borders -noteUnit wide)
			double w = (it->end - it->begin) * pixUnit - m_noteUnit * 2.0; // width: including borders on both sides
			float hh = -m_noteUnit;
			float centery = m_baseY + (it->note + 0.4) * m_noteUnit; // Star is 0.4 notes higher than current note
			float centerx = x + w - (player_star_offset + 1.2) * hh; // Star is 1.2 units from end
			float rot = std::remainder(time * 5.0, TAU); // They rotate!
			bool smallerNoteGraph = ((position == NoteGraph::TOP) || (position == NoteGraph::BOTTOM));
			float zoom = (std::abs((rot-180) / 360.0f) * 0.8f + 0.6f) * (smallerNoteGraph ? 2.3 : 2.0) * hh;
			using namespace glmath;
			Transform trans(translate(vec3(centerx, centery, 0.0f)) * rotate(rot, vec3(0.0f, 0.0f, 1.0f)));
			{
				ColorTrans c(Color(it_col->r, it_col->g, it_col->b, it_col->a));
				m_star_hl.draw(Dimensions().stretch(zoom*1.2, zoom*1.2).center().middle(), TexCoords());
			}
			m_star.draw(Dimensions().stretch(zoom, zoom).center().middle(), TexCoords());
			player_star_offset += 0.8;
		}
	}
}

void NoteGraph::drawNotes() {
	// Draw note lines
	m_notelines.draw(Dimensions().stretch(dimensions.w(), (m_max - m_min - 13) * m_noteUnit).middle(dimensions.xc()).center(dimensions.yc()), TexCoords(0.0, (-m_min - 7.0) / 12.0f, 1.0, (-m_max + 6.0) / 12.0f));

	// Draw notes
	for (auto it = m_songit; it != m_vocal.notes.end() && it->begin < m_time - (baseLine - 0.5) / pixUnit; ++it) {
		if (it->type == Note::SLEEP) continue;
		double alpha = it->power;
		Texture* t1;
		Texture* t2;
		switch (it->type) {
			case Note::NORMAL: case Note::SLIDE: t1 = &m_notebar; t2 = &m_notebar_hl; break;
			case Note::GOLDEN:
			case Note::GOLDEN2: //fallthrough
				t1 = &m_notebargold; t2 = &m_notebargold_hl;
			break;
			case Note::FREESTYLE:  // Freestyle notes use custom handling
			case Note::RAP: //handle RAP notes like freestyle for now
			{
				Dimensions dim;
				dim.middle(m_baseX + 0.5 * (it->begin + it->end) * pixUnit).center(m_baseY + it->note * m_noteUnit).stretch((it->end - it->begin) * pixUnit, -m_noteUnit * 12.0);
				float xoffset = 0.1 * m_time / m_notebarfs.dimensions.ar();
				m_notebarfs.draw(dim, TexCoords(xoffset, 0.0, xoffset + dim.ar() / m_notebarfs.dimensions.ar(), 1.0));
				if (alpha > 0.0) {
					float xoffset = rand() / double(RAND_MAX);
					m_notebarfs_hl.draw(dim, TexCoords(xoffset, 0.0, xoffset + dim.ar() / m_notebarfs_hl.dimensions.ar(), 1.0));
				}
			}
			continue;
		  default: throw std::logic_error("Unknown note type: don't know how to render");
		}
		double x = m_baseX + it->begin * pixUnit + m_noteUnit; // left x coordinate: begin minus border (side borders -noteUnit wide)
		double bar_height = barHeight();
		double ybeg = m_baseY + (it->notePrev +bar_height) * m_noteUnit; // top y coordinate (on the one higher note line)
		double yend = m_baseY + (it->note +bar_height) * m_noteUnit; // top y coordinate (on the one higher note line)
		double w = (it->end - it->begin) * pixUnit - m_noteUnit * 2.0; // width: including borders on both sides
		double h_x = -m_noteUnit * 2.0; // height: 0.5 border + 1.0 bar + 0.5 border = 2.0
		double h_y = h_x * bar_height; //
		drawNotebar(*t1, x, ybeg, yend, w, h_x, h_y);
		if (alpha > 0.0) {
			ColorTrans c(Color::alpha(alpha));
			drawNotebar(*t2, x, ybeg, yend, w, h_x, h_y);
		}
	}
}

double NoteGraph::barHeight() {
	switch(config["game/difficulty"].i()){
		case 0:
			return 1;
		case 1:
			return 0.5;
		case 2:
			return 0.21;
	}
	return 1;
}

namespace {
	void strip(glutil::VertexArray& va) {
		if (va.size() > 3) va.draw();
		va.clear();
	}
}

void NoteGraph::drawWaves(Database const& database) {
	if (m_vocal.notes.empty()) return; // Cannot draw without notes
	UseTexture tblock(m_wave);
	for (auto const& player: database.cur) {
		if (player.m_vocal.name != m_vocal.name)
			continue;
		float const texOffset = 2.0 * m_time; // Offset for animating the wave texture
		Player::pitch_t const& pitch = player.m_pitch;
		size_t const beginIdx = std::max(0.0, m_time - 0.5 / pixUnit) / Engine::TIMESTEP; // At which pitch idx to start displaying the wave
		size_t const endIdx = player.m_pos;
		size_t idx = beginIdx;
		// Go back until silence (NaN freq) to allow proper wave phase to be calculated
		if (beginIdx < endIdx) while (idx > 0 && pitch[idx].first == pitch[idx].first) --idx;
		// Start processing
		float tex = texOffset;
		double t = idx * Engine::TIMESTEP;
		double oldval = getNaN();
		glutil::VertexArray va;
		auto noteIt = m_vocal.notes.begin();
		glmath::vec4 c(player.m_color.r, player.m_color.g, player.m_color.b, 1.0);
		for (; idx < endIdx; ++idx, t += Engine::TIMESTEP) {
			double const freq = pitch[idx].first;
			// If freq is NaN, we have nothing to process
			if (freq != freq) { oldval = getNaN(); tex = texOffset; continue; }
			tex += freq * 0.001; // Wave phase (texture coordinate)
			if (idx < beginIdx) continue; // Skip graphics rendering if out of screen
			double x = -0.2 + (t - m_time) * pixUnit;
			// Find the currently active note(s)
			while (noteIt != m_vocal.notes.end() && (noteIt->type == Note::SLEEP || t > noteIt->end)) ++noteIt;
			auto notePrev = noteIt;
			while (notePrev != m_vocal.notes.begin() && (notePrev->type == Note::SLEEP || t < notePrev->begin)) --notePrev;
			bool hasNote = (noteIt != m_vocal.notes.end());
			bool hasPrev = notePrev->type != Note::SLEEP && t >= notePrev->begin;
			double val;
			if (hasNote && hasPrev) val = 0.5 * (noteIt->note + notePrev->note);
			else if (hasNote) val = noteIt->note;
			else val = notePrev->note;
			// Now val contains the active note value. The following calculates note value for current freq:
			val += Note::diff(val, MusicalScale(m_vocal.scale).setFreq(freq).getNote());
			// Graphics positioning & animation:
			double y = m_baseY + val * m_noteUnit;
			double thickness = clamp(1.0 + pitch[idx].second / 60.0) + 0.5;
			thickness *= barHeight() * (1.0 + 0.2 * std::sin(tex - 2.0 * texOffset)); // Further animation :)
			thickness *= -m_noteUnit;
			// If there has been a break or if the pitch change is too fast, terminate and begin a new one
			if (oldval != oldval || std::abs(oldval - val) > 1) strip(va);
			// Add a point or a pair of points
			if (!va.size()) va.texCoord(tex, 0.5f).color(c).vertex(x, y);
			else {
				va.texCoord(tex, 0.0f).color(c).vertex(x, y - thickness);
				va.texCoord(tex, 1.0f).color(c).vertex(x, y + thickness);
			}
			oldval = val;
		}
		strip(va);
	}
}

