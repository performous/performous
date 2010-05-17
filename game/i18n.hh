#pragma once

#ifdef USE_GETTEXT
/* Internationalization Dependances */
#include <libintl.h>
#include <locale.h>
#define _(x) gettext(x)
#else
#define _(x) (x)
#endif

class Gettext {
  public:
	Gettext(const char *package) {
#ifdef USE_GETTEXT
		// initialize gettext
#ifdef _MSC_VER
		setlocale(LC_ALL, "");//only untill we don't have a better solution. This because LC_MESSAGES cause crash under Visual Studio
#else
		setlocale (LC_MESSAGES, "");
#endif
		bindtextdomain (package, getLocaleDir().string().c_str());
		textdomain (package);
		bind_textdomain_codeset (package, "UTF-8");
#endif
	};
	static bool enabled() {
		#ifdef USE_GETTEXT
		return true;
		#else
		return false;
		#endif
	}
};
