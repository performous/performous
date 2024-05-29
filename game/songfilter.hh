#pragma once

#include "isongfilter.hh"

#include <set>
#include <string>
#include <vector>

class AndFilter : public ISongFilter {
public:
	virtual ~AndFilter() override = default;

	AndFilter& add(SongFilterPtr const& filter) {
		m_filters.emplace_back(filter);

		return *this;
	}

	bool isEmpty() const {
		return m_filters.empty();
	}

	bool filter(Song const& song) const override {
		for(auto const& filter : m_filters)
			if(!filter->filter(song))
				return false;

		return true;
	}

	std::string toString() const override {
		auto s = std::string{"&("};

		for(auto const& child : m_filters)
			s += child->toString();
		s += ")";

		return s;
	}

private:
	std::vector<SongFilterPtr> m_filters;
};

class OrFilter : public ISongFilter {
public:
	virtual ~OrFilter() override = default;

	OrFilter& add(SongFilterPtr const& filter) {
		m_filters.emplace_back(filter);

		return *this;
	}

	bool isEmpty() const {
		return m_filters.empty();
	}

	bool filter(Song const& song) const override {
		for(auto const& filter : m_filters)
			if(filter->filter(song))
				return true;

		return false;
	}

	std::string toString() const override {
		auto s = std::string{"|("};

		for(auto const& child : m_filters)
			s += child->toString();
		s += ")";

		return s;
	}

private:
	std::vector<SongFilterPtr> m_filters;
};

class GenreFilter : public ISongFilter {
public:
	GenreFilter(std::string const&, std::set<std::string> const& genres);

	bool filter(Song const&) const override;

	std::string toString() const override {
		return "genre=" + m_genre;
	}

private:
	std::string m_genre;
	std::set<std::string> m_genres;
};

class LanguageFilter : public ISongFilter {
public:
	LanguageFilter(std::string const&);

	bool filter(Song const&) const override;

	std::string toString() const override {
		return "lang=" + m_language;
	}

private:
	std::string m_language;
};

class ArtistFilter : public ISongFilter {
public:
	ArtistFilter(std::string const&);

	bool filter(Song const&) const override;

	std::string toString() const override {
		return "artist=" + m_artist;
	}

private:
	std::string m_artist;
};

class TitleFilter : public ISongFilter {
public:
	TitleFilter(std::string const&);

	bool filter(Song const&) const override;

	std::string toString() const override {
		return "title=" + m_title;
	}

private:
	std::string m_title;
};

class YearFilter : public ISongFilter {
public:
	YearFilter(size_t from, size_t to);

	bool filter(Song const&) const override;

	std::string toString() const override {
		return "year=" + std::to_string(m_from) + "-" + std::to_string(m_to);
	}

private:
	size_t m_from;
	size_t m_to;
};

class ModeFilter : public ISongFilter {
public:
	enum class Mode {SoloVocals, DuetVocals, Dance, Drums, Keyboard, Guitar};

	ModeFilter(Mode mode);

	bool filter(Song const&) const override;

	std::string toString() const override {
		return "mode=?";
	}

private:
	Mode m_mode;
};


class BrokenFilter : public ISongFilter {
public:
	BrokenFilter(bool broken);

	bool filter(Song const&) const override;

	std::string toString() const override {
		return std::string{ "broken=" } + (m_broken ? "true" : "false");
	}

private:
	bool m_broken;
};

