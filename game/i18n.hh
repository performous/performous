#pragma once

#include <boost/locale.hpp>
#include <iostream>
#include "fs.hh"
using namespace std;
using namespace boost::locale;

#define _(x) translate(x).str()
#define translate_noop(x) x

class TranslationEngine {
	public:
		TranslationEngine(const char *package) {
			generator gen;
		    gen.add_messages_path(getLocaleDir().string());
		    gen.add_messages_domain(package);
		    locale::global(gen(""));
		    (void)package;		    
		};
		static bool enabled() {
			return true;
		};

};