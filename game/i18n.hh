#pragma once

#include <boost/locale.hpp>
#include <iostream>
#include <string>
#include <map>

#include "configuration.hh"
#include "fs.hh"

#define _(x) boost::locale::translate(x).str()
#define translate_noop(x) x

class TranslationEngine {
public:
	TranslationEngine(const char *package) : m_package(package) {

		initializeAllLanguages();
		populateLanguages(GetAllLanguages());

		std::clog << "locale/debug: Checking if language is set in config." << std::endl;

		auto prefValue = config["game/language"].getEnumName();
		auto lang = getLanguageByHumanReadableName(prefValue);

		setLanguage(lang);
	};
	static bool enabled() {
		return true;
	};

	static ConfigItem& TranslationEngine::translationConfig() {
		static ConfigItem& backend = config["game/language"];
		return backend;
	}

	void TranslationEngine::initializeAllLanguages() {
		auto path = getLocaleDir().string();
		m_gen.add_messages_path(path);
		m_gen.add_messages_domain(m_package);
		m_gen.locale_cache_enabled(true);

		for (auto const& language : GetAllLanguages()) {
			if (language.first == "None") continue;
			m_gen(language.first);
		}
	}

	void setLanguage(const std::string& language, bool fromSettings = false) {
		std::clog << "locale/notice: Setting language to: '" << language << "'" << std::endl;

		if (fromSettings) {
			m_currentLanguage = { getLanguageByHumanReadableName(language) , language };
		}
		else {
			m_currentLanguage = { language , getLanguageByKey(language) };
		}

		try {
			std::locale::global(m_gen(m_currentLanguage.first));
			std::cout << "locale/notice: Current language is: '" << m_currentLanguage.second << "'" << std::endl;
			populateLanguages(GetAllLanguages(true), true);
		}
		catch (...) {
			std::clog << "locale/warning: Unable to detect locale, will try to fallback to en_US.UTF-8" << std::endl;
			std::locale::global(m_gen("en_US.UTF-8"));
		}
	}

	std::string getLanguageByHumanReadableName(const std::string& language) {
		if (language == "Auto") {
			return boost::locale::util::get_system_locale(true);
		}

		auto allLanguages = GetAllLanguages();	
		auto languageKey = std::string();
		for (auto it = allLanguages.begin(); it != allLanguages.end(); ++it) {
			if (it->second == language) {
				languageKey = it->first;
				break;
			}
		}

		return languageKey;
	}

	std::string getLanguageByKey(const std::string& languageKey) {
		if (languageKey == "None") {
			return boost::locale::util::get_system_locale(true);
		}

		auto allLanguages = GetAllLanguages();
		if (allLanguages.find(languageKey) == allLanguages.end()) {
			return allLanguages["en_US.UTF-8"];
		}
		return allLanguages.at(languageKey);
	}

	const std::pair<std::string, std::string> getCurrentLanguage() { return m_currentLanguage; }

	std::map<std::string, std::string> GetAllLanguages(bool refresh = false) {
		if (refresh) {
			m_languages.clear();
		}
		if (m_languages.size() != 0) return m_languages;
		m_languages.emplace(std::pair("None", _("Auto")));
		m_languages.emplace(std::pair("ast_ES.UTF-8", _("Asturian")));
		m_languages.emplace(std::pair("da_DK.UTF-8", _("Danish")));
		m_languages.emplace(std::pair("de_DE.UTF-8", _("German")));
		m_languages.emplace(std::pair("en_US.UTF-8", _("English")));
		m_languages.emplace(std::pair("es_ES.UTF-8", _("Spanish")));
		m_languages.emplace(std::pair("fa_IR.UTF-8", _("Persian")));
		m_languages.emplace(std::pair("fi_FI.UTF-8", _("Finnish")));
		m_languages.emplace(std::pair("fr_FR.UTF-8", _("French")));
		m_languages.emplace(std::pair("hu_HU.UTF-8", _("Hungarian")));
		m_languages.emplace(std::pair("it_IT.UTF-8", _("Italian")));
		m_languages.emplace(std::pair("ja_JP.UTF-8", _("Japanese")));
		m_languages.emplace(std::pair("nl_NL.UTF-8", _("Dutch")));
		m_languages.emplace(std::pair("pl_PL.UTF-8", _("Polish")));
		m_languages.emplace(std::pair("pt_PT.UTF-8", _("Portuguese")));
		m_languages.emplace(std::pair("sv_SE.UTF-8", _("Swedish")));
		m_languages.emplace(std::pair("zh_CN.UTF-8", _("Chinese")));
		return m_languages;
	}

private:
	std::pair<std::string, std::string> m_currentLanguage;
	std::string m_package;
	boost::locale::generator m_gen;
	std::map<std::string, std::string> m_languages;
};