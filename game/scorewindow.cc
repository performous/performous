#include "scorewindow.hh"

#include "database.hh"

ScoreWindow::ScoreWindow(Instruments& instruments, Database& database):
  m_database(database),
  m_pos(0.8f, 2.0f),
  m_bg(findFile("score_window.svg")),
  m_scoreBar(findFile("score_bar_bg.svg"), findFile("score_bar_fg.svg"), ProgressBar::Mode::VERTICAL, 0.0f, 0.0f, false),
  m_score_text(findFile("score_txt.svg")),
  m_score_rank(findFile("score_rank.svg"))
{
	Game::getSingletonPtr()->showLogo();
	m_pos.setTarget(0.0);
	m_database.scores.clear();
	// Singers
	for (auto p = m_database.cur.begin(); p != m_database.cur.end();) {
		const auto score = p->getScore();

		if (score < 500) { // Dead
			p = m_database.cur.erase(p);
			continue;
		}

		m_database.scores.emplace_back(score, input::DevType::VOCALS, "Vocals", "vocals", Color(p->m_color.r, p->m_color.g, p->m_color.b));
		++p;
	}
	// Instruments
	for (auto it = instruments.begin(); it != instruments.end();) {
		const auto score = (*it)->getScore();

		if (score < 100) { // Dead
			it = instruments.erase(it);
			continue;
		}

		const auto type = (*it)->getGraphType();
		const auto track_simple = (*it)->getTrack();
		const auto track = UnicodeUtil::toUpper((*it)->getModeId(), 1); // Capitalize
		auto color = Color(1.0f, 0.0f, 0.0f);

		if (track_simple == TrackName::DRUMS)
			color = Color(0.1f, 0.1f, 0.1f);
		else if (track_simple == TrackName::BASS)
			color = Color(0.5f, 0.3f, 0.1f);

		m_database.scores.emplace_back(score, type, track, track_simple, color);
		++it;
	}

	if (m_database.scores.empty())
		m_rank = _("No player!");
	else {
		// Determine winner
		m_database.scores.sort([](ScoreItem i, ScoreItem j) -> bool { return (i.score>j.score); });
		const auto winner = *std::max_element(m_database.scores.begin(), m_database.scores.end());
		const auto topScore = winner.score;
		// Determine rank
		if (winner.type == input::DevType::VOCALS) {
			if (topScore > 8000) m_rank = _("Hit singer");
			else if (topScore > 6000) m_rank = _("Lead singer");
			else if (topScore > 4000) m_rank = _("Rising star");
			else if (topScore > 2000) m_rank = _("Amateur");
			else m_rank = _("Tone deaf");
		} else if (winner.type == input::DevType::DANCEPAD) {
			if (topScore > 8000) m_rank = _("Maniac");
			else if (topScore > 6000) m_rank = _("Hoofer");
			else if (topScore > 4000) m_rank = _("Rising star");
			else if (topScore > 2000) m_rank = _("Amateur");
			else m_rank = _("Loser");
		} else {
			if (topScore > 8000) m_rank = _("Virtuoso");
			else if (topScore > 6000) m_rank = _("Rocker");
			else if (topScore > 4000) m_rank = _("Rising star");
			else if (topScore > 2000) m_rank = _("Amateur");
			else m_rank = _("Tone deaf");
		}
	}
	m_bg.dimensions.middle().center();
}

void ScoreWindow::draw() {
	using namespace glmath;
	Transform trans(translate(vec3(0.0f, static_cast<float>(m_pos.get()), 0.0f)));
	m_bg.draw();
	const float spacing = 0.1f + 0.1f / m_database.scores.size();
	unsigned i = 0;

	for (auto p = m_database.scores.begin(); p != m_database.scores.end(); ++p, ++i) {
		unsigned score = p->score;
		ColorTrans c(p->color);
		float x = spacing * (0.5f + i - 0.5f * m_database.scores.size());
		m_scoreBar.dimensions.fixedWidth(0.09f).middle(x).bottom(0.20f);
		m_scoreBar.draw(score / 10000.0f);
		m_score_text.render(std::to_string(score));
		m_score_text.dimensions().middle(x).top(0.24f).fixedHeight(0.042f);
		m_score_text.draw();
		m_score_text.render(p->track_simple);
		m_score_text.dimensions().middle(x).top(0.20f).fixedHeight(0.042f);
		m_score_text.draw();
	}
	m_score_rank.draw(m_rank);
}

bool ScoreWindow::empty() {
	return m_database.scores.empty();
}

