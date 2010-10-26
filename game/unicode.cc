#include "unicode.hh"

#include <boost/scoped_ptr.hpp>
#include <glibmm/ustring.h>
#include <glib/gconvert.h>
#include <sstream>
#include <stdexcept>

namespace {
	// Convert a string using Glib, throw exception on error.
	// This is in fact (slightly modified) Glib::convert from glibmm.
	std::string convert(const std::string& str, const std::string& to_codeset, const std::string& from_codeset) {
		gsize bytes_written = 0;
		GError* gerror = 0;

		char *const buf = g_convert(
		  str.data(), str.size(), to_codeset.c_str(), from_codeset.c_str(),
		  0, &bytes_written, &gerror);

		if (gerror) throw std::runtime_error("Conversion error"); // Throw on error

		return std::string(boost::scoped_ptr<char>(buf).get(), bytes_written);
	}
}

void convertToUTF8( std::stringstream &_stream, std::string _filename ) {
	try {
		convert(_stream.str(), "UTF-8", "UTF-8"); // Test if input is UTF-8
	} catch(...) {
		if (!_filename.empty()) std::clog << "unicode/warning: " << _filename << " is not UTF-8.\n  Assuming CP1252 for now. Use recode CP1252..UTF-8 */*.txt to convert your files." << std::endl;
		try {
			_stream.str(convert(_stream.str(), "UTF-8", "CP1252")); // Convert from Microsoft CP1252
		} catch (...) {
			// Filter out anything but ASCII
			std::string tmp;
			for (char ch; _stream.get(ch);) tmp += (ch >= 0x20 && ch < 0x7F) ? ch : '?';
		}
	}
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
