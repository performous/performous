#include "unicode.hh"
#include "configuration.hh"

#include <boost/scoped_ptr.hpp>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>
#include <sstream>
#include <stdexcept>

void convertToUTF8(std::stringstream &_stream, std::string _filename) {
	try {
		std::string data = _stream.str();
		Glib::convert(data, "UTF-8", "UTF-8"); // Test if input is UTF-8
		if (data.substr(0, 3) == "\xEF\xBB\xBF") {
			std::clog << "unicode/warning: " << _filename << " UTF-8 BOM ignored. Please avoid editors that use BOMs (e.g. Notepad)." << std::endl;
			_stream.str(data.substr(3)); // Remove BOM if there is one
		}
	} catch(...) {
		std::string codeset = config["game/fallback_encoding"].getEnumName();
		if (!_filename.empty())
			std::clog << "unicode/warning: " << _filename << " is not UTF-8.\n  Assuming " << codeset
				<< ". Use recode " << codeset << "..UTF-8 */*.txt to convert your files." << std::endl;
		try {
			_stream.str(Glib::convert(_stream.str(), "UTF-8", codeset)); // Convert from fallback encoding
		} catch (...) {
			// Filter out anything but ASCII
			std::string tmp;
			for (char ch; _stream.get(ch);) tmp += (ch >= 0x20 && ch < 0x7F) ? ch : '?';
		}
	}
}

std::string convertToUTF8(std::string const& str) {
	std::stringstream ss(str);
	convertToUTF8(ss);
	return ss.str();
}

std::string unicodeCollate(std::string const& str) {
	Glib::ustring ustr = str, ustr2;
	if (ustr.substr(0, 4) == "The ") ustr = ustr.substr(4) + "the";
	if (ustr.substr(0, 2) == "A ") ustr = ustr.substr(2) + "a";
	// Remove all non-alnum characters
	for (Glib::ustring::iterator it = ustr.begin(), end = ustr.end(); it != end; ++it) {
		if (Glib::Unicode::isalnum(*it)) ustr2 += Glib::Unicode::tolower(*it);
	}
	return ustr2;
	// Should use ustr2.casefold_collate_key() instead of tolower, but it seems to be crashing...
}
