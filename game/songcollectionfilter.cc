#include "songcollectionfilter.hh"

#include "unicode.hh"

#include <algorithm>
#include <iostream>

#include <unicode/stsearch.h>


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
			if (m_type == FilterType::HasDance && !song->hasDance()) return true;
			if (m_type == FilterType::HasVocals && !song->hasVocals()) return true;
			if (m_type == FilterType::HasDuet && !song->hasDuet()) return true;
			if (m_type == FilterType::HasGuitar && !song->hasGuitars()) return true;
			if (m_type == FilterType::HasDrumsOrKeyboard && (!song->hasDrums() && !song->hasKeyboard())) return true;
			if (m_type == FilterType::FullBand && (!song->hasVocals() || !song->hasGuitars() || (!song->hasDrums() && !song->hasKeyboard()))) return true;

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
