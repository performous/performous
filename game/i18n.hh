#pragma once

#include <boost/locale.hpp>
#include <unicode/locid.h>

#include <iostream>
#include <string>
#include <map>
#include <memory>

#define _(x) boost::locale::translate(x).str()
#define translate_noop(x) x

class TranslationEngine {
public:
	TranslationEngine();

	static void initializeAllLanguages();
	static void setLanguage(const std::string& language, bool fromSettings = false);
	static std::string getLanguageByHumanReadableName(const std::string& language);
	static std::string getLanguageByKey(const std::string& languageKey);
	static std::string getCurrentLanguageCode();
	static std::pair<std::string, std::string> const& getCurrentLanguage();
	static std::map<std::string, std::string> GetAllLanguages(bool refresh = false);
	static icu::Locale& getIcuLocale() { return *m_icuLocale; }

private:
	static std::vector<std::string> getLocalePaths();
	static std::pair<std::string, std::string> m_currentLanguage;
	static std::string m_package;
	static boost::locale::generator m_gen;
	static std::unique_ptr<icu::Locale> m_icuLocale;
	static std::map<std::string, std::string> m_languages;
};
