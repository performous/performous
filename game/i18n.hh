#pragma once

#ifdef HAVE_GETTEXT
/* Internationalization Dependances */
#include <libintl.h>
#include <locale.h>
#define _(x) gettext(x)
#else
#define _(x) (x)
#endif

