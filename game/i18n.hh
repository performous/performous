#pragma once

#include <boost/locale.hpp>
#include <iostream>
#include <string>
#include <map>

#define _(x) boost::locale::translate(x).str()
#define translate_noop(x) x

class TranslationEngine {
public:
	TranslationEngine(const char *package);;

	void initializeAllLanguages();
	void setLanguage(const std::string& language, bool fromSettings = false);
	std::string getLanguageByHumanReadableName(const std::string& language);
	std::string getLanguageByKey(const std::string& languageKey);
	const std::pair<std::string, std::string>& getCurrentLanguage() const;
	std::map<std::string, std::string> GetAllLanguages(bool refresh = false);

private:
	std::vector<std::string> getLocalePaths() const;
    
private:
	std::pair<std::string, std::string> m_currentLanguage;
	std::string m_package;
	boost::locale::generator m_gen;
	std::map<std::string, std::string> m_languages;
};
