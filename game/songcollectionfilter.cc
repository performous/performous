#include "songcollectionfilter.hh"

#include "unicode.hh"

#include <algorithm>
#include <iostream>

#include <unicode/stsearch.h>

namespace {
    bool filterByType(Song const& song, FilterType type) {
        switch (type) {
        case FilterType::HasDance:
            return !song.hasDance();
        case FilterType::HasVocals:
            return !song.hasVocals();
        case FilterType::HasDuet:
            return !song.hasDuet();
        case FilterType::HasGuitar:
            return !song.hasGuitars();
        case FilterType::HasDrumsOrKeyboard:
            return !song.hasDrums() && !song.hasKeyboard();
        case FilterType::FullBand:
            return !song.hasVocals() || !song.hasGuitars() || (!song.hasDrums() && !song.hasKeyboard());
        default:
            return false;
        }
    }
}

SongCollectionFilter::SongCollection SongCollectionFilter::filter(SongCollection const& collection) const {
	auto filtered = collection;

	if(m_filter) {
		std::cout << "filter by filter" << std::endl;

		auto const n = std::erase_if(filtered, [&](auto const& song) { return !m_filter->filter(*song);});

		std::cout << "erased " << n << " of " << collection.size() << " songs" << std::endl;
	}

	if (!m_filterString.empty() || m_type != FilterType::None) {
		std::cout << "filter by string or type" << std::endl;
		auto filter = icu::UnicodeString::fromUTF8(UnicodeUtil::convertToUTF8(m_filterString));
		icu::ErrorCode icuError;

		std::erase_if (filtered, [&](auto const& song){
			// Filter by type first.
			if (filterByType(*song, m_type))
				return true;

			// If search is not empty, filter by search term.
			if (!m_filterString.empty()) {
				icu::StringSearch search = icu::StringSearch(filter, icu::UnicodeString::fromUTF8(song->strFull()), UnicodeUtil::m_searchCollator.get(), nullptr, icuError);

				return search.first(icuError) == USEARCH_DONE;
			}

			// If we still haven't returned, it must be a type match with an empty search string.
			return false;
		});
	}

	return filtered;
}

SongCollectionFilter& SongCollectionFilter::setFilter(SongFilterPtr filter) {
	m_filter = filter;

	return *this;
}

SongCollectionFilter& SongCollectionFilter::setFilter(std::string const& filter) {
	m_filterString = filter;

	return *this;
}

SongCollectionFilter& SongCollectionFilter::setType(FilterType const& type) {
	m_type = type;

	return *this;
}
