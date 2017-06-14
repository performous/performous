#pragma once

#include <boost/locale.hpp>
#include <iostream>
#include "fs.hh"
using namespace std;
using namespace boost::locale;

#define _(x) translate(x).str()

class Gettext {
	public:
		Gettext(const char *package) {
			generator gen;
		    // Specify location of dictionaries
		    gen.add_messages_path(getLocaleDir().c_str());
		    gen.add_messages_domain(package);
		    // Generate locales and imbue them to iostream
		    locale::global(gen(""));
		    (void)package;
		};
		static bool enabled() {
			return true;
		};

};