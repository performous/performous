#include "notegraph.hh"

#include "configuration.hh"
#include "database.hh"
#include "engine.hh"
#include "player.hh"
#include "graphic/color_trans.hh"
#include "graphic/texture_manager.hh"
#include "graphic/transform.hh"

Dimensions dimensions; // Make a public member variable

NoteGraph::NoteGraph(VocalTrack const& vocal, NoteGraphScalerPtr const& scaler, TextureManager& textureManager):
	m_vocal(vocal),
	m_notelines(textureManager.get(findFile("notelines.svg"))),
	m_wave(textureManager.get(findFile("wave.svg"))),
	m_star(textureManager.get(findFile("star.svg"))),
	m_star_hl(textureManager.get(findFile("star_glow.svg"))),
	m_notebar(textureManager.get(findFile("notebar.svg"))),
	m_notebar_hl(textureManager.get(findFile("notebar_hi.svg"))),
	m_notebarfs(textureManager.get(findFile("notebarfs.svg"))),
	m_notebarfs_hl(textureManager.get(findFile("notebarfs_hi.svg"))),
	m_notebargold(textureManager.get(findFile("notebargold.svg"))),
	m_notebargold_hl(textureManager.get(findFile("notebargold_hi.svg"))),
	m_scaler(scaler)
{
	dimensions.stretch(1.0f, 0.5f); // Initial dimensions, probably overridden from somewhere
	m_nlTop.setTarget(m_vocal.noteMax, true);
	m_nlBottom.setTarget(m_vocal.noteMin, true);
	for (auto const& n: m_vocal.notes)
		n.stars.clear(); // Reset stars
	reset();

	m_scaler->initialize(vocal);
}

void NoteGraph::reset() {
	m_songit = m_vocal.notes.begin();
}

namespace {
	void drawNotebar(Window& window, Texture const& texture, float x, float ybeg, float yend, float w, float h_x, float h_y) {
		glutil::VertexArray va;
		TextureBinder tblock(window, texture);

		// The front cap begins
		va.texCoord(0.0f, 0.0f).vertex(x, ybeg);
		va.texCoord(0.0f, 1.0f).vertex(x, ybeg + h_y);
		if (w >= 2.0f * h_x) {
			// Calculate the y coordinates of the middle part
			float tmp = h_x / w;  // h_x = cap size (because it is a h_x by h_x square)
			float y1 = (1.0f - tmp) * ybeg + tmp * yend;
			float y2 = tmp * ybeg + (1.0f - tmp) * yend;
			// The middle part between caps
			va.texCoord(0.5f, 0.0f).vertex(x + h_x, y1);
			va.texCoord(0.5f, 1.0f).vertex(x + h_x, y1 + h_y);
			va.texCoord(0.5f, 0.0f).vertex(x + w - h_x, y2);
			va.texCoord(0.5f, 1.0f).vertex(x + w - h_x, y2 + h_y);
		} else {
			// Note is too short to even fit caps, crop to fit.
			float ymid = 0.5f * (ybeg + yend);
			float crop = 0.25f * w / h_x;
			va.texCoord(crop, 0.0f).vertex(x + 0.5f * w, ymid);
			va.texCoord(crop, 1.0f).vertex(x + 0.5f * w, ymid + h_y);
			va.texCoord(1.0f - crop, 0.0f).vertex(x + 0.5f * w, ymid);
			va.texCoord(1.0f - crop, 1.0f).vertex(x + 0.5f * w, ymid + h_y);
		}
		// The rear cap ends
		va.texCoord(1.0f, 0.0f).vertex(x + w, yend);
		va.texCoord(1.0f, 1.0f).vertex(x + w, yend + h_y);

		va.draw();
	}
}

const float baseLine = -0.2f;
const float pixUnit = 0.2f;

void NoteGraph::draw(Window& window, double time, Database const& database, Position position) {
	if (time < m_time)
		reset();
	m_time = time;
	// Update m_songit (which note to start the rendering from)
	while (m_songit != m_vocal.notes.end() && (m_songit->type == Note::Type::SLEEP || m_songit->end < time - (baseLine + 0.5f) / pixUnit))
		++m_songit;

	// Automatically zooming notelines
	{
		const auto dimensions = m_scaler->calculate(m_vocal, m_songit, time);

		if (dimensions.min2 <= dimensions.max2) {
			m_nlTop.setRange(dimensions.max2, dimensions.max1);
			m_nlBottom.setRange(dimensions.min1, dimensions.min2);
		}
	}
	switch(position) {
		case NoteGraph::Position::FULLSCREEN:
			dimensions.stretch(1.0f, 0.50f).center();
			break;
		case NoteGraph::Position::TOP:
			dimensions.stretch(1.0f, 0.32f).bottom(0.0f);
			break;
		case NoteGraph::Position::BOTTOM:
			dimensions.stretch(1.0f, 0.32f).top(0.0f);
			break;
		case NoteGraph::Position::LEFT:
			dimensions.stretch(0.50f, 0.50f).center().left(-0.5f);
			break;
		case NoteGraph::Position::RIGHT:
			dimensions.stretch(0.50f, 0.50f).center().right();
			break;
	}
	m_max = static_cast<float>(m_nlTop.get() + 7.0f);
	m_min = static_cast<float>(m_nlBottom.get() - 7.0f);
	m_noteUnit = -dimensions.h() / std::max(48.0f * dimensions.h(), m_max - m_min);
	m_baseY = -0.5f * (m_min + m_max) * m_noteUnit + dimensions.yc();
	m_baseX = static_cast<float>(baseLine - m_time * pixUnit + dimensions.xc());  // FIXME: Moving in X direction requires additional love (is b0rked now, keep it centered at zero)

	// Fading notelines handing
	if (m_songit == m_vocal.notes.end() || m_songit->begin > m_time + 3.0)
		m_notealpha -= 0.02f;
	else if (m_notealpha < 1.0f)
		m_notealpha += 0.02f;
	if (m_notealpha <= 0.0f) {
		m_notealpha = 0.0f;
		return;
	}

	ColorTrans c(window, Color::alpha(m_notealpha));

	drawNotes(window);
	if (config["game/pitch"].b())
		drawWaves(window, database);

	// Draw a star for well sung notes
	for (auto it = m_songit; it != m_vocal.notes.end() && it->begin < m_time - (baseLine - 0.5f) / pixUnit; ++it) {
		float player_star_offset = 0;
		for (std::vector<Color>::const_iterator it_col = it->stars.begin(); it_col != it->stars.end(); ++it_col) {
			float x = static_cast<float>(m_baseX + it->begin * pixUnit + m_noteUnit); // left x coordinate: begin minus border (side borders -noteUnit wide)
			float w = static_cast<float>((it->end - it->begin) * pixUnit - m_noteUnit * 2.0f); // width: including borders on both sides
			float hh = -m_noteUnit;
			float centery = m_baseY + (it->note + 0.4f) * m_noteUnit; // Star is 0.4 notes higher than current note
			float centerx = x + w - (player_star_offset + 1.2f) * hh; // Star is 1.2 units from end
			float rot = static_cast<float>(std::remainder(time * 5.0, TAU)); // They rotate!
			bool smallerNoteGraph = ((position == NoteGraph::Position::TOP) || (position == NoteGraph::Position::BOTTOM));
			float zoom = (std::abs((rot-180) / 360.0f) * 0.8f + 0.6f) * (smallerNoteGraph ? 2.3f : 2.0f) * hh;
			using namespace glmath;
			Transform trans(window, translate(vec3(centerx, centery, 0.0f)) * rotate(rot, vec3(0.0f, 0.0f, 1.0f)));
			{
				ColorTrans c(window, Color(it_col->r, it_col->g, it_col->b, it_col->a));
				m_star_hl.draw(window, Dimensions().stretch(zoom*1.2f, zoom*1.2f).center().middle(), TexCoords());
			}
			m_star.draw(window, Dimensions().stretch(zoom, zoom).center().middle(), TexCoords());
			player_star_offset += 0.8f;
		}
	}
}

void NoteGraph::drawNotes(Window& window) {
	// Draw note lines
	m_notelines.draw(window, Dimensions().stretch(dimensions.w(), (m_max - m_min - 13) * m_noteUnit).middle(dimensions.xc()).center(dimensions.yc()), TexCoords(0.0f, (-m_min - 7.0f) / 12.0f, 1.0f, (-m_max + 6.0f) / 12.0f));

	// Draw notes
	for (auto it = m_songit; it != m_vocal.notes.end() && it->begin < m_time - (baseLine - 0.5f) / pixUnit; ++it) {
		if (it->type == Note::Type::SLEEP)
			continue;
		float alpha = it->power;
		Texture* t1;
		Texture* t2;
		switch (it->type) {
			case Note::Type::NORMAL:
			case Note::Type::SLIDE:
				t1 = &m_notebar; t2 = &m_notebar_hl;
			break;
			case Note::Type::GOLDEN:
			case Note::Type::GOLDEN2: //fallthrough
				t1 = &m_notebargold; t2 = &m_notebargold_hl;
			break;
			case Note::Type::FREESTYLE:  // Freestyle notes use custom handling
			case Note::Type::RAP: //handle RAP notes like freestyle for now
			{
				Dimensions dim;
				dim.middle(static_cast<float>(m_baseX + 0.5f * (it->begin + it->end) * pixUnit)).center(static_cast<float>(m_baseY + it->note * m_noteUnit)).stretch(static_cast<float>(it->end - it->begin) * pixUnit, -m_noteUnit * 12.0f);
				float xoffset = static_cast<float>(0.1 * m_time / m_notebarfs.dimensions.ar());
				m_notebarfs.draw(window, dim, TexCoords(xoffset, 0.0f, xoffset + dim.ar() / m_notebarfs.dimensions.ar(), 1.0f));
				if (alpha > 0.0f) {
					float xoffset = static_cast<float>(rand() / double(RAND_MAX));
					m_notebarfs_hl.draw(window, dim, TexCoords(xoffset, 0.0f, xoffset + dim.ar() / m_notebarfs_hl.dimensions.ar(), 1.0f));
				}
			}
			continue;
			case Note::Type::SLEEP:
			case Note::Type::TAP:
			case Note::Type::HOLDBEGIN:
			case Note::Type::HOLDEND:
			case Note::Type::ROLL:
			case Note::Type::MINE:
			case Note::Type::LIFT:
			default:
				throw std::logic_error("Unknown note type: don't know how to render");
		}
		float x = static_cast<float>(m_baseX + it->begin * pixUnit + m_noteUnit); // left x coordinate: begin minus border (side borders -noteUnit wide)
		float bar_height = barHeight();
		float ybeg = m_baseY + (it->notePrev +bar_height) * m_noteUnit; // top y coordinate (on the one higher note line)
		float yend = m_baseY + (it->note +bar_height) * m_noteUnit; // top y coordinate (on the one higher note line)
		float w = static_cast<float>(it->end - it->begin) * pixUnit - m_noteUnit * 2.0f; // width: including borders on both sides
		float h_x = -m_noteUnit * 2.0f; // height: 0.5 border + 1.0 bar + 0.5 border = 2.0
		float h_y = h_x * bar_height; //
		drawNotebar(window, *t1, x, ybeg, yend, w, h_x, h_y);
		if (alpha > 0.0f) {
			ColorTrans c(window, Color::alpha(alpha));
			drawNotebar(window, *t2, x, ybeg, yend, w, h_x, h_y);
		}
	}
}

float NoteGraph::barHeight() {
	return static_cast<float>(2.0 * thresholdForFullScore());
}

float NoteGraph::waveThickness(){
	return static_cast<float>(thresholdForNonzeroScore() - thresholdForFullScore());
}

namespace {
	void strip(glutil::VertexArray& va) {
		if (va.size() > 3)
			va.draw();
		va.clear();
	}
}

void NoteGraph::drawWaves(Window& window, Database const& database) {
	if (m_vocal.notes.empty())
		return; // Cannot draw without notes
	TextureBinder tblock(window, m_wave);
	auto sortedPlayers = std::list<std::reference_wrapper<const Player>>(database.cur.begin(), database.cur.end());
	sortedPlayers.sort([](const Player& playerOne, const Player& playerTwo) {
		return playerOne.m_score < playerTwo.m_score;
	});
	for (const Player& player: sortedPlayers) {
		if (player.m_vocal.name != m_vocal.name)
			continue;
		float const texOffset = static_cast<float>(2.0 * m_time); // Offset for animating the wave texture
		Player::pitch_t const& pitch = player.m_pitch;
		size_t const beginIdx = static_cast<size_t>(std::max(0.0, m_time - 0.5 / pixUnit) / Engine::TIMESTEP); // At which pitch idx to start displaying the wave
		size_t const endIdx = player.m_pos;
		size_t idx = beginIdx;
		// Go back until silence (NaN freq) to allow proper wave phase to be calculated
		if (beginIdx < endIdx)
			while (idx > 0 && pitch[idx].first == pitch[idx].first)
				--idx;
		// Start processing
		float tex = texOffset;
		double t = static_cast<double>(idx) * Engine::TIMESTEP;
		double oldval = getNaN();
		glutil::VertexArray va;
		auto noteIt = m_vocal.notes.begin();
		glmath::vec4 c(player.m_color.r, player.m_color.g, player.m_color.b, 1.0f);
		for (; idx < endIdx; ++idx, t += Engine::TIMESTEP) {
			double const freq = pitch[idx].first;
			// If freq is NaN, we have nothing to process
			if (freq != freq) {
				oldval = getNaN();
				tex = texOffset;
				continue;
			}
			tex += static_cast<float>(freq * 0.001); // Wave phase (texture coordinate)
			if (idx < beginIdx)
				continue; // Skip graphics rendering if out of screen
			float x = static_cast<float>(-0.2f + (t - m_time) * pixUnit);
			// Find the currently active note(s)
			while (noteIt != m_vocal.notes.end() && (noteIt->type == Note::Type::SLEEP || t > noteIt->end))
				++noteIt;
			auto notePrev = noteIt;
			while (notePrev != m_vocal.notes.begin() && (notePrev->type == Note::Type::SLEEP || t < notePrev->begin))
				--notePrev;
			bool hasNote = (noteIt != m_vocal.notes.end());
			bool hasPrev = notePrev->type != Note::Type::SLEEP && t >= notePrev->begin;
			double val;
			if (hasNote && hasPrev)
				val = 0.5 * (noteIt->note + notePrev->note);
			else if (hasNote)
				val = noteIt->note;
			else
				val = notePrev->note;
			// Now val contains the active note value. The following calculates note value for current freq:
			val += Note::diff(val, MusicalScale(m_vocal.scale).setFreq(freq).getNote());
			// Graphics positioning & animation:
			float y = static_cast<float>(m_baseY + val * m_noteUnit);
			double thickness = clamp(1.0 + pitch[idx].second / 60.0) + 0.5;
			thickness *= NoteGraph::waveThickness() * (1.0 + 0.2 * std::sin(tex - 2.0 * texOffset)); // Further animation :)
			thickness *= -m_noteUnit;
			// If there has been a break or if the pitch change is too fast, terminate and begin a new one
			if (oldval != oldval || std::abs(oldval - val) > 1)
				strip(va);
			// Add a point or a pair of points
			if (!va.size())
				va.texCoord(tex, 0.5f).color(c).vertex(x, y);
			else {
				va.texCoord(tex, 0.0f).color(c).vertex(x, y - static_cast<float>(thickness));
				va.texCoord(tex, 1.0f).color(c).vertex(x, y + static_cast<float>(thickness));
			}
			oldval = val;
		}
		strip(va);
	}
}
