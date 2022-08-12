#include "i18n.hh"

#include "config.hh"
#include "configuration.hh"
#include "fs.hh"
#include "unicode.hh"

#include <unicode/locid.h>

std::pair<std::string, std::string> TranslationEngine::m_currentLanguage{"en_US.UTF-8", "English" };
std::string TranslationEngine::m_package = PACKAGE;
boost::locale::generator TranslationEngine::m_gen{};
std::map<std::string, std::string> TranslationEngine::m_languages{};

TranslationEngine::TranslationEngine() {
	initializeAllLanguages();
	/* set all languages in configuration.
	 * They are kept untranslated internally which prevents having issues
	 * when changing languages (and need to update internal stuff with
	 * translations) */
	populateLanguages(GetAllLanguages());

	std::clog << "locale/debug: Checking if language is set in config." << std::endl;

	auto prefValue = config["game/language"].getEnumName();
	auto lang = getLanguageByHumanReadableName(prefValue);

	setLanguage(lang);
}

void TranslationEngine::initializeAllLanguages() {
	for(auto const& path : getLocalePaths()) {
		std::cout << "add locale path: " << path << std::endl;
		m_gen.add_messages_path(path);
	}
        
	m_gen.add_messages_domain(m_package);
	m_gen.locale_cache_enabled(true);

	for (auto const& language : GetAllLanguages()) {
		if (language.first == "None") continue;
		m_gen(language.first);
	}
}

void TranslationEngine::setLanguage(const std::string& language, bool fromSettings) {
	std::clog << "locale/notice: Setting language to: '" << language << "'" << std::endl;

	if (fromSettings) {
		TranslationEngine::m_currentLanguage = { getLanguageByHumanReadableName(language) , language };
	}
	else {
		TranslationEngine::m_currentLanguage = { language , getLanguageByKey(language) };
	}
	auto searchLocale = icu::Locale::createCanonical("en_US.UTF-8");
	auto sortLocale = icu::Locale::createCanonical("en_US.UTF-8");
	icu::ErrorCode error;
	try {
		searchLocale = icu::Locale::createCanonical(m_currentLanguage.first.c_str());
		sortLocale = icu::Locale::createCanonical(m_currentLanguage.first.c_str());
#if U_ICU_VERSION_MAJOR_NUM >= 63
		sortLocale.setUnicodeKeywordValue("co","standard", error);
#else
		sortLocale.setKeywordValue("collation","standard", error);
#endif
		if (error.isFailure()) throw std::runtime_error("Error " + std::to_string(error.get()) + " creating sorting locale: " + error.errorName());
		error.reset();
#if U_ICU_VERSION_MAJOR_NUM >= 63
		searchLocale.setUnicodeKeywordValue("co","search", error);
#else
		sortLocale.setKeywordValue("collation","search", error);
#endif
		if (error.isFailure()) throw std::runtime_error("Error " + std::to_string(error.get()) + " creating search locale: " + error.errorName());
		std::locale::global(m_gen(m_currentLanguage.first));
		std::cout << "locale/notice: Current language is: '" << m_currentLanguage.second << "'" << std::endl;
	}
	catch (std::runtime_error& e) {
		std::clog << "locale/warning: Unable to detect locale, will try to fallback to en_US.UTF-8. Exception: " << e.what() << std::endl;
		std::locale::global(m_gen("en_US.UTF-8"));
	}

	icu::RuleBasedCollator* search;
	icu::RuleBasedCollator* sort;

	error.reset();	
	search = dynamic_cast<icu::RuleBasedCollator*>(icu::RuleBasedCollator::createInstance(searchLocale, error));
	if (!search || error.isFailure()) throw std::runtime_error("Unable to create search collator. error: " + std::to_string(error.get()) + ": " + error.errorName());

	error.reset();
	sort = dynamic_cast<icu::RuleBasedCollator*>(icu::RuleBasedCollator::createInstance(sortLocale, error));
	if (!sort || error.isFailure()) throw std::runtime_error("Unable to create search collator. error: " + 
std::to_string(error.get()) + ": " + error.errorName());

	UnicodeUtil::m_searchCollator.reset(search);
	UnicodeUtil::m_sortCollator.reset(sort);
	UnicodeUtil::m_searchCollator->setStrength(icu::Collator::PRIMARY);
	UnicodeUtil::m_sortCollator->setStrength(icu::Collator::SECONDARY);
}

std::string TranslationEngine::getLanguageByHumanReadableName(const std::string& language) {
	if (language == "Auto") {
		return boost::locale::util::get_system_locale(true);
	}

	auto allLanguages = GetAllLanguages();	
	std::string languageKey;
	for (auto const& lang : allLanguages) {
		if (lang.second == language) {
			languageKey = lang.first;
			break;
		}
	}

	return languageKey;
}

std::string TranslationEngine::getLanguageByKey(const std::string& languageKey) {
	if (languageKey == "None") {
		return boost::locale::util::get_system_locale(true);
	}
	auto allLanguages = GetAllLanguages();
	if (allLanguages.find(languageKey) == allLanguages.end()) {
		return allLanguages["en_US.UTF-8"];
	}
	return allLanguages.at(languageKey);
}

std::pair<std::string, std::string> const& TranslationEngine::getCurrentLanguage() { 
	return TranslationEngine::m_currentLanguage; 
}

std::string TranslationEngine::getCurrentLanguageCode() {
	std::string lang(getCurrentLanguage().first);
	return lang.substr(0, lang.find('.'));
}

std::map<std::string, std::string> TranslationEngine::GetAllLanguages(bool refresh) {
	if (refresh) {
		TranslationEngine::m_languages.clear();
	}
	if (TranslationEngine::m_languages.size() != 0) return TranslationEngine::m_languages;

	// Internally, all strings are kept in English, but they are eventually
	// displayed as menu entries thus they need translation
	TranslationEngine::m_languages = {
		{ "None", translate_noop("Auto") },
		{ "ast_ES.UTF-8", translate_noop("Asturian") },
		{ "da_DK.UTF-8", translate_noop("Danish") },
		{ "de_DE.UTF-8", translate_noop("German") },
		{ "en_US.UTF-8", translate_noop("English") },
		{ "es_ES.UTF-8", translate_noop("Spanish") },
		{ "fa_IR.UTF-8", translate_noop("Persian") },
		{ "fi_FI.UTF-8", translate_noop("Finnish") },
		{ "fr_FR.UTF-8", translate_noop("French") },
		{ "hu_HU.UTF-8", translate_noop("Hungarian") },
		{ "it_IT.UTF-8", translate_noop("Italian") },
		{ "ja_JP.UTF-8", translate_noop("Japanese") },
		{ "nl_NL.UTF-8", translate_noop("Dutch") },
		{ "pl_PL.UTF-8", translate_noop("Polish") },
		{ "pt_PT.UTF-8", translate_noop("Portuguese") },
		{ "sk_SK.UTF-8", translate_noop("Slovak") },
		{ "sv_SE.UTF-8", translate_noop("Swedish") },
		{ "zh_CN.UTF-8", translate_noop("Chinese") }
	};

	return m_languages;
}

std::vector<std::string> TranslationEngine::getLocalePaths() {
	auto paths = std::vector<std::string>{ ".", "./lang" };
        
	auto const* root = getenv("PERFORMOUS_ROOT");
        
	if(root) {
		paths.emplace_back(root);
		paths.emplace_back(root + std::string{"/lang"});
	}
        
	auto const path = getLocaleDir().string();
        
	if(!path.empty())
		paths.emplace_back(path);
        
	return paths;
}

