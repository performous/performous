#include "scorewindow.hh"

#include "database.hh"

#include <algorithm>

ScoreWindow::ScoreWindow(Game& game, Instruments& instruments, Database& database):
  m_game(game),
  m_database(database),
  m_pos(0.8f, 2.0f),
  m_bg(findFile("score_window.svg")),
  m_scoreBar(findFile("score_bar_bg.svg"), findFile("score_bar_fg.svg"), ProgressBar::Mode::VERTICAL, 0.0f, 0.0f, false),
  m_score_text(findFile("score_txt.svg")),
  m_score_rank(findFile("score_rank.svg"))
{
	game.showLogo();
	m_pos.setTarget(0.0);
	m_database.scores.clear();

	// Singers
	m_database.cur.remove_if([](Player const& p){ return p.getScore() < 500; }); // Dead.
	for (Player const& p: m_database.cur) {
		m_database.scores.emplace_back(p.getScore(), input::DevType::VOCALS, "Vocals", "vocals", Color(p.m_color.r, p.m_color.g, p.m_color.b));
	}

	// Instruments
	std::remove_if(instruments.begin(), instruments.end(),[](std::unique_ptr<InstrumentGraph> const& i){ return i->getScore() < 100; }); // Dead.
	for (std::unique_ptr<InstrumentGraph> const& i: instruments) {
		input::DevType const& type = i->getGraphType();
		std::string const& track_simple = i->getTrack();
		std::string const& track = UnicodeUtil::toTitle(i->getModeId()); // Capitalize
		Color color = Color(1.0f, 0.0f, 0.0f);

		if (track_simple == TrackName::DRUMS)
			color = Color(0.1f, 0.1f, 0.1f);
		else if (track_simple == TrackName::BASS)
			color = Color(0.5f, 0.3f, 0.1f);

		m_database.scores.emplace_back(i->getScore(), type, track, track_simple, color);
	}

	if (m_database.scores.empty())
		m_rank = _("No player!");
	else {
		// Determine winner
		m_database.scores.sort([](ScoreItem i, ScoreItem j) -> bool { return (i.score>j.score); });
		ScoreItem const& winner = *std::max_element(m_database.scores.begin(), m_database.scores.end());
		const unsigned topScore = winner.score;
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
	auto& window = m_game.getWindow();
	Transform trans(window, translate(vec3(0.0f, static_cast<float>(m_pos.get()), 0.0f)));
	m_bg.draw(window);
	const float spacing = 0.1f + 0.1f / static_cast<float>(m_database.scores.size());
	unsigned i = 0;
	for (ScoreItem const& s: m_database.scores) {
		unsigned score = s.score;
		ColorTrans c(window, s.color);
		float x = spacing * (0.5f + static_cast<float>(i) - 0.5f * static_cast<float>(m_database.scores.size()));
		m_scoreBar.dimensions.fixedWidth(0.09f).middle(x).bottom(0.20f);
		m_scoreBar.draw(window, static_cast<float>(score) / 10000.0f);
		m_score_text.render(std::to_string(score));
		m_score_text.dimensions().middle(x).top(0.24f).fixedHeight(0.042f);
		m_score_text.draw(window);
		m_score_text.render(s.track_simple);
		m_score_text.dimensions().middle(x).top(0.20f).fixedHeight(0.042f);
		m_score_text.draw(window);
		++i;
	}
	m_score_rank.draw(window, m_rank);
}

bool ScoreWindow::empty() {
	return m_database.scores.empty();
}

