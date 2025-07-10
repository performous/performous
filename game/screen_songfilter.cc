#include "screen_songfilter.hh"
#include "screen_songfilter.hh"

#include "fs.hh"
#include "i18n.hh"
#include "game.hh"
#include "songs.hh"
#include "songfilter.hh"
#include "util.hh"

#include <algorithm>

namespace {
	const std::set<std::string> genreTerms{
		"Alternative", "Country", "Rock", "Pop", "Metal", "Punk", "Folk", "Grunge",
		"Reggae", "Dance", "House", "Soul", "Musical", "Ballad", "Soundtrack", "Blues",
		"New age", "R&B", "Oldie", "Ska",
		"NDW"};

	struct SongsSummery {
		std::vector<std::string> languages;
		std::vector<std::string> genres;
	};

	std::set<std::string> findMatch(std::string const& word, std::set<std::string> const& terms) {
		auto result = std::set<std::string>();

		for(auto const& term : terms) {
			if(containsNoCase(word, term))
				result.emplace(term);
		}

		if(result.empty())
			result.emplace("Other");

		return result;
	}

	SongsSummery collectSummery(Songs const& songs) {
		const auto none = "<" + _("None") + ">";
		auto result = SongsSummery();

		result.languages.emplace_back(none);
		result.genres.emplace_back(none);

		auto languages = std::set<std::string>();
		auto genres = std::set<std::string>();

		for(auto const& song : songs.getSongs()) {
			languages.emplace(song->language);
//			std::cout << song->genre << std::endl;
			genres.merge(findMatch(song->genre, genreTerms));
		}

		std::copy(languages.begin(), languages.end(), std::back_inserter(result.languages));
		std::copy(genres.begin(), genres.end(), std::back_inserter(result.genres));

		return result;
	}
}

ScreenSongFilter::ScreenSongFilter(Game& game, Songs& songs)
: FormScreen(game, "SongFilter"), m_game(game), m_songs(songs) {
	initializeControls();
}

void ScreenSongFilter::onExitSwitchTo(std::string const& screen) {
	m_nextScreen = screen;
}

void ScreenSongFilter::initializeControls() {
	const auto songSummery = collectSummery(m_songs);
	const auto verticalSpace = 0.05f;
	const auto verticalOffset = -0.15f;
	const auto horizontalSpace = 0.225f;
	const auto horizontalOffset = -0.45f;
	const auto lineHeight = 0.04f;
	const auto none = "<" + _("None") + ">";
	auto n = 0.f;

	getForm().addControl(m_labelLanguage);
	m_labelLanguage.setText(_("Language") + ":");
	m_labelLanguage.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);
	getForm().addControl(m_selectLanguage0);
	m_selectLanguage0.setItems(songSummery.languages);
	m_selectLanguage0.select(0);
	m_selectLanguage0.setGeometry(horizontalOffset + horizontalSpace, verticalOffset + n * verticalSpace, 0.2f, lineHeight);
	getForm().addControl(m_selectLanguage1);
	m_selectLanguage1.setItems(songSummery.languages);
	m_selectLanguage1.select(0);
	m_selectLanguage1.setGeometry(horizontalOffset + horizontalSpace * 2, verticalOffset + n * verticalSpace, 0.2f, lineHeight);
	getForm().addControl(m_selectLanguage2);
	m_selectLanguage2.setItems(songSummery.languages);
	m_selectLanguage2.select(0);
	m_selectLanguage2.setGeometry(horizontalOffset + horizontalSpace * 3, verticalOffset + n * verticalSpace, 0.2f, lineHeight);

	++n;

	getForm().addControl(m_labelGenre);
	m_labelGenre.setText(_("Genre") + ":");
	m_labelGenre.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);
	getForm().addControl(m_selectGenre0);
	m_selectGenre0.setItems(songSummery.genres);
	m_selectGenre0.select(0);
	m_selectGenre0.setGeometry(horizontalOffset + horizontalSpace, verticalOffset + n * verticalSpace, 0.2f, lineHeight);
	getForm().addControl(m_selectGenre1);
	m_selectGenre1.setItems(songSummery.genres);
	m_selectGenre1.select(0);
	m_selectGenre1.setGeometry(horizontalOffset + horizontalSpace * 2, verticalOffset + n * verticalSpace, 0.2f, lineHeight);
	getForm().addControl(m_selectGenre2);
	m_selectGenre2.setItems(songSummery.genres);
	m_selectGenre2.select(0);
	m_selectGenre2.setGeometry(horizontalOffset + horizontalSpace * 3, verticalOffset + n * verticalSpace, 0.2f, lineHeight);

	++n;

	getForm().addControl(m_labelYear);
	m_labelYear.setText(_("Year") + ":");
	m_labelYear.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);
	getForm().addControl(m_selectYear0);
	m_selectYear0.setItems({none, "1960s", "1970s", "1980s", "1990s", "> 2000"});
	m_selectYear0.select(0);
	m_selectYear0.setGeometry(horizontalOffset + horizontalSpace, verticalOffset + n * verticalSpace, 0.2f, lineHeight);
	getForm().addControl(m_selectYear1);
	m_selectYear1.setItems({none, "1960s", "1970s", "1980s", "1990s", "> 2000"});
	m_selectYear1.select(0);
	m_selectYear1.setGeometry(horizontalOffset + horizontalSpace * 2, verticalOffset + n * verticalSpace, 0.2f, lineHeight);
	getForm().addControl(m_selectYear2);
	m_selectYear2.setItems({none, "1960s", "1970s", "1980s", "1990s", "> 2000"});
	m_selectYear2.select(0);
	m_selectYear2.setGeometry(horizontalOffset + horizontalSpace * 3, verticalOffset + n * verticalSpace, 0.2f, lineHeight);

	++n;

	getForm().addControl(m_labelMode);
	m_labelMode.setText(_("Mode") + ":");
	m_labelMode.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);
	getForm().addControl(m_selectMode);
	m_selectMode.setItems({none, "Solo", "Duet", "Dance", "Drums", "Keyboard", "Guitar"});
	m_selectMode.select(0);
	m_selectMode.setGeometry(horizontalOffset + horizontalSpace, verticalOffset + n * verticalSpace, 0.2f, lineHeight);

	++n;

	getForm().addControl(m_labelTitle);
	m_labelTitle.setText(_("Title") + ":");
	m_labelTitle.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);
	getForm().addControl(m_textBoxTitle);
	m_textBoxTitle.setGeometry(horizontalOffset + horizontalSpace, verticalOffset + n * verticalSpace, 0.2f + horizontalSpace * 2, lineHeight);
	m_textBoxTitle.setMaxLength(24);

	++n;

	getForm().addControl(m_labelArtist);
	m_labelArtist.setText(_("Artist") + ":");
	m_labelArtist.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);
	getForm().addControl(m_textBoxArtist);
	m_textBoxArtist.setGeometry(horizontalOffset + horizontalSpace, verticalOffset + n * verticalSpace, 0.2f + horizontalSpace * 2, lineHeight);
	m_textBoxArtist.setMaxLength(24);

	++n;

	getForm().addControl(m_labelBroken);
	m_labelBroken.setText(_("Misc") + ":");
	m_labelBroken.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);
	getForm().addControl(m_selectBroken);
	m_selectBroken.setItems({ none, "Correct", "Broken" });
	m_selectBroken.select(0);
	m_selectBroken.setGeometry(horizontalOffset + horizontalSpace, verticalOffset + n * verticalSpace, 0.2f, lineHeight);

	++n;
	//++n;
	//++n;
	//++n;

	getForm().addControl(m_labelResults);
	m_labelResults.setGeometry(horizontalOffset, verticalOffset + n * verticalSpace, horizontalSpace, lineHeight);

	getForm().addControl(m_buttonReset);
	m_buttonReset.setText(_("Reset"));
	m_buttonReset.setGeometry(horizontalOffset + horizontalSpace * 3, verticalOffset + n * verticalSpace, 0.2f, lineHeight);
	m_buttonReset.onClicked([this](auto&) { resetFilter(); });

	getForm().focusNext();
}

namespace {
	SongFilterPtr makeYearFilter(size_t n) {
		switch(n) {
			case 1:
				return std::make_shared<YearFilter>(1960, 1969);
			case 2:
				return std::make_shared<YearFilter>(1970, 1979);
			case 3:
				return std::make_shared<YearFilter>(1980, 1989);
			case 4:
				return std::make_shared<YearFilter>(1990, 1999);
			case 5:
				return std::make_shared<YearFilter>(2000, 2999);
			default:
				return std::make_shared<YearFilter>(0, 2999);
		}
	}
}

SongFilterPtr ScreenSongFilter::makeFilter() const {
	auto result = std::make_shared<AndFilter>();
	auto languageFilter = std::make_shared<OrFilter>();

	std::cout << "selected " << m_selectLanguage0.getSelectedIndex() << std::endl;
	std::cout << "selected " << m_selectLanguage0.getSelectedText() << std::endl;
	if(m_selectLanguage0.getSelectedIndex() != 0)
		languageFilter->add(std::make_shared<LanguageFilter>(m_selectLanguage0.getSelectedText()));
	if(m_selectLanguage1.getSelectedIndex() != 0)
		languageFilter->add(std::make_shared<LanguageFilter>(m_selectLanguage1.getSelectedText()));
	if(m_selectLanguage2.getSelectedIndex() != 0)
		languageFilter->add(std::make_shared<LanguageFilter>(m_selectLanguage2.getSelectedText()));

	if(!languageFilter->isEmpty())
		result->add(languageFilter);

	auto genreFilter = std::make_shared<OrFilter>();

	if(m_selectGenre0.getSelectedIndex() != 0)
		genreFilter->add(std::make_shared<GenreFilter>(m_selectGenre0.getSelectedText(), genreTerms));
	if(m_selectGenre1.getSelectedIndex() != 0)
		genreFilter->add(std::make_shared<GenreFilter>(m_selectGenre1.getSelectedText(), genreTerms));
	if(m_selectGenre2.getSelectedIndex() != 0)
		genreFilter->add(std::make_shared<GenreFilter>(m_selectGenre2.getSelectedText(), genreTerms));

	if(!genreFilter->isEmpty())
		result->add(genreFilter);

	auto yearFilter = std::make_shared<OrFilter>();

	if(m_selectYear0.getSelectedIndex() != 0)
		yearFilter->add(makeYearFilter(m_selectYear0.getSelectedIndex()));
	if(m_selectYear1.getSelectedIndex() != 0)
		yearFilter->add(makeYearFilter(m_selectYear1.getSelectedIndex()));
	if(m_selectYear2.getSelectedIndex() != 0)
		yearFilter->add(makeYearFilter(m_selectYear2.getSelectedIndex()));

	if(!yearFilter->isEmpty())
		result->add(yearFilter);

	if(m_selectMode.getSelectedIndex() != 0)
		result->add(std::make_shared<ModeFilter>(static_cast<ModeFilter::Mode>(m_selectMode.getSelectedIndex() - 1)));

	if(!m_textBoxTitle.getText().empty())
		result->add(std::make_shared<TitleFilter>(m_textBoxTitle.getText()));

	if(!m_textBoxArtist.getText().empty())
		result->add(std::make_shared<ArtistFilter>(m_textBoxArtist.getText()));

	if (m_selectBroken.getSelectedIndex() != 0)
		result->add(std::make_shared<BrokenFilter>(m_selectBroken.getSelectedIndex() == 2));

	return result;
}

void ScreenSongFilter::updateResult() {
	const auto filter = makeFilter();
	const auto songs = m_songs.getSongs();
	const auto all = songs.size();
	const auto filtered = std::count_if(songs.begin(), songs.end(), [filter](auto const& song) { return filter->filter(*song);});
	auto const text = _("Result") + ": " + std::to_string(filtered) + " / " + std::to_string(all);

	m_labelResults.setText(text);

	std::cout << "text: " << text << std::endl;
	std::cout << "filter: " << filter->toString() << std::endl;

	m_songs.setFilter(filter);
}

void ScreenSongFilter::resetFilter() {
	m_selectLanguage0.select(0);
	m_selectLanguage1.select(0);
	m_selectLanguage2.select(0);
	m_selectGenre0.select(0);
	m_selectGenre1.select(0);
	m_selectGenre2.select(0);
	m_selectYear0.select(0);
	m_selectYear1.select(0);
	m_selectYear2.select(0);
	m_selectMode.select(0);
	m_textBoxArtist.setText({});
	m_textBoxTitle.setText({});
	m_selectBroken.select(0);

	updateResult();
}

void ScreenSongFilter::onCancel() {
	m_game.activateScreen(m_nextScreen);
}

void ScreenSongFilter::onAfterEventProcessing() {
	updateResult();
}

void ScreenSongFilter::draw() {
	m_theme->bg.draw(m_game.getWindow());
	FormScreen::draw();
}

void ScreenSongFilter::enter() {
	m_theme = std::make_unique<ThemeSongFilterScreen>();
}

void ScreenSongFilter::exit() {
	m_theme.reset();
}
