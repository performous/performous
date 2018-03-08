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
		std::locale::global(gen(""));
	};
	static bool enabled() {
		return true;
	};

};