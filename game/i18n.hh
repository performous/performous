#pragma once

#include <boost/locale.hpp>
#include <iostream>
#include "fs.hh"

#define _(x) boost::locale::translate(x).str()
#define translate_noop(x) x

class TranslationEngine {
public:
	TranslationEngine(const char *package) {
		boost::locale::generator gen;
		gen.add_messages_path(getLocaleDir().string());
		gen.add_messages_domain(package);
		try {
			std::locale::global(gen(""));
		} catch (...) {
			std::clog << "locale/warning: Unable to detect locale, will try to fallback to en_US.UTF-8" << std::endl;
			std::locale::global(gen("en_US.UTF-8"));
		}
	};
	static bool enabled() {
		return true;
	};

};