#include "filtertype.hh"

#include "i18n.hh"

#include <stdexcept>

std::ostream& operator<<(std::ostream& stream, FilterType const& type) {
	switch (type) {
		case FilterType::None: return stream << _("show all songs");
		case FilterType::HasDance: return stream << _("has dance");
		case FilterType::HasVocals: return stream << _("has vocals");
		case FilterType::HasDuet: return stream << _("has duet");
		case FilterType::HasGuitar: return stream << _("has guitar");
		case FilterType::HasDrumsOrKeyboard: return stream << _("drums or keytar");
		case FilterType::FullBand: return stream << _("full band");
	}

	throw std::logic_error("Internal error: unknown filter type");
}

