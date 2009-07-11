#include "unicode.hh"

#include <glibmm/ustring.h>
#include <glibmm/convert.h>

namespace Unicode {
	void convert( std::stringstream &_stream, std::string _filename ) {
		try {
			Glib::convert(_stream.str(), "UTF-8", "UTF-8"); // Test if input is UTF-8
		} catch(...) {
			if( _filename.empty() ) _filename = std::string("stream");

			std::cerr << "WARNING: " << _filename << " is not UTF-8.\n  Assuming CP1252 for now. Use recode CP1252..UTF-8 */*.txt to convert your files." << std::endl;
			try {
				_stream.str(Glib::convert(_stream.str(), "UTF-8", "CP1252")); // Convert from Microsoft CP1252
			} catch (...) {
				// Filter out anything but ASCII
				std::string tmp;
				for (char ch; _stream.get(ch);) tmp += (ch >= 0x20 && ch < 0x7F) ? ch : '?';
			}
		}
	}

	std::string collate(std::string const& str) {
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
}
