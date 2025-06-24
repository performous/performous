#pragma once

#include "song.hh"
#include "filtertype.hh"
#include "isongfilter.hh"

#include <string>
#include <vector>

class SongCollectionFilter {
  public:
	using SongCollection = std::vector<SongPtr>;

	SongCollection filter(SongCollection const&) const;

	SongCollectionFilter& setFilter(SongFilterPtr);
	SongCollectionFilter& setFilter(std::string const&);
	SongCollectionFilter& setType(FilterType const&);

  private:
	SongFilterPtr m_filter;
	std::string m_filterString;
	FilterType m_type = FilterType::None;
};
