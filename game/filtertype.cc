#include "filtertype.hh"

#include "i18n.hh"

#include <stdexcept>

int toInt(FilterType type) {
	return static_cast<int>(type);
}

FilterType toFilterType(int type) {
	return static_cast<FilterType>(type);
}

std::string toString(FilterType  const& type) {
	switch (type) {
		case FilterType::None: return _("show all songs");
		case FilterType::HasDance: return _("has dance");
		case FilterType::HasVocals: return _("has vocals");
		case FilterType::HasDuet: return _("has duet");
		case FilterType::HasGuitar: return _("has guitar");
		case FilterType::HasDrumsOrKeyboard: return _("drums or keytar");
		case FilterType::FullBand: return _("full band");
	}

	throw std::logic_error("Internal error: unknown filter type");
}

std::ostream& operator<<(std::ostream& stream, FilterType const& type) {
	return stream << toString(type);
}

