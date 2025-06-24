#include "songfilter.hh"

#include "song.hh"
#include "util.hh"

#include <iostream>

namespace {
	bool matches(std::string const& word, std::set<std::string> const& terms) {
		for(auto const& term : terms) {
			if(containsNoCase(word, term))
				return true;
		}

		return false;
	}
}

GenreFilter::GenreFilter(std::string const& genre, std::set<std::string> const& genres)
: m_genre(genre), m_genres(genres) {
}

bool GenreFilter::filter(Song const& song) const {
	if(matches(song.genre, m_genres))
		return containsNoCase(song.genre, m_genre);

	return m_genre == "Other";
}


LanguageFilter::LanguageFilter(std::string const& language)
: m_language(language) {
	//std::cout << "set language " << language << std::endl;
}

bool LanguageFilter::filter(Song const& song) const {
	//std::cout << "compare " << song.language << " with " << m_language << std::endl;
	return song.language == m_language;
}


ArtistFilter::ArtistFilter(std::string const& artist)
: m_artist(toLower(artist)) {
}

bool ArtistFilter::filter(Song const& song) const {
	if(m_artist.empty())
		return true;

	const auto artist = toLower(song.artist);

	return artist.find(m_artist) != std::string::npos;
}


TitleFilter::TitleFilter(std::string const& title)
: m_title(toLower(title)) {
}

bool TitleFilter::filter(Song const& song) const {
	if(m_title.empty())
		return true;

	const auto title = toLower(song.title);

	return title.find(m_title) != std::string::npos;
}


YearFilter::YearFilter(size_t from, size_t to)
: m_from(from), m_to(to) {
}

bool YearFilter::filter(Song const& song) const {
	if(song.getYear() == 0)
		return true;

	return m_from <= song.getYear() && song.getYear() <= m_to;
}


ModeFilter::ModeFilter(ModeFilter::Mode mode)
: m_mode(mode) {
}

bool ModeFilter::filter(Song const& song) const {
	switch(m_mode) {
		case Mode::SoloVocals:
			return song.hasSoloVocals();
		case Mode::DuetVocals:
			return song.hasDuetVocals();
		case Mode::Dance:
			return song.hasDance();
		case Mode::Drums:
			return song.hasDrums();
		case Mode::Keyboard:
			return song.hasKeyboard();
		case Mode::Guitar:
			return song.hasGuitars();
	}

	return false;
}

BrokenFilter::BrokenFilter(bool broken)
	: m_broken(broken) {
}

bool BrokenFilter::filter(Song const& song) const {
	return song.isBroken() == m_broken;
}


