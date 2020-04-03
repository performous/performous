#pragma once

#include <boost/locale.hpp>
#include <iostream>

#include "fs.hh"
#include "configuration.hh"

#define _(x) boost::locale::translate(x).str()
#define translate_noop(x) x

class TranslationEngine {
public:
	TranslationEngine(const char *package) : m_package(package) {
		auto lang = boost::locale::util::get_system_locale(true);

		std::clog << "locale/debug: Checking if language is set in config." << std::endl;
		if(config["game/language"].i() >= 0) {
			lang = config["game/language"].getEnumStringValueAtIndex(config["game/language"].i());
			std::clog << "locale/debug: Language in configuration found: " << lang << std::endl;
		}

		if (lang.empty()) lang = "en_US.UTF-8";
		setLanguage(lang);
	};
	static bool enabled() {
		return true;
	};

	void setLanguage(const std::string language) { 
		auto path = getLocaleDir().native();
		boost::locale::generator gen;
		gen.add_messages_path(path);
		gen.add_messages_domain(m_package);
		gen.locale_cache_enabled(true);

		auto theChosenOne = gen(language);

		std::clog << "locale/notice: Setting language to: '" << language << "'" << std::endl;
		m_currentLanguage = getLanguage(language);

		try {
			std::locale::global(theChosenOne);
			std::cout << "locale/notice: Current language is: '" << m_currentLanguage << "'" << std::endl;
		} catch (...) {
			std::clog << "locale/warning: Unable to detect locale, will try to fallback to en_US.UTF-8" << std::endl;
			std::locale::global(gen("en_US.UTF-8"));
		}
	}

	const std::string getLanguage(const std::string language) { 
		if(language.empty()) {
			auto lan = boost::locale::util::get_system_locale(true);
			std::clog << "locale/notice: No given language.. using default system language: '" << lan << "''" << std::endl;
			return lan;
		}

		auto allLanguages = GetAllLanguages();
		auto foundLanguage = std::find(allLanguages.begin(), allLanguages.end(), language);
		auto returnLang = std::string();

		if (foundLanguage == allLanguages.end())
			returnLang = "en_US.UTF-8";
		else 
			returnLang = *foundLanguage;

		return returnLang;
	}

	const std::string getCurrentLanguage() { return m_currentLanguage; }
	std::vector<std::string> GetAllLanguages() { return config["game/language"].getAllEnumStringValues(); }
private:
	std::string m_currentLanguage;
	const char *m_package;
};