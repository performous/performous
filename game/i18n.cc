#include "i18n.hh"

#include "configuration.hh"
#include "fs.hh"

TranslationEngine::TranslationEngine(const char *package) : m_package(package) {
	initializeAllLanguages();
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
	catch (std::runtime_error& e) {
		std::clog << "locale/warning: Unable to detect locale, will try to fallback to en_US.UTF-8. Exception: " << e.what() << std::endl;
		std::locale::global(m_gen("en_US.UTF-8"));
	}
}

std::string TranslationEngine::getLanguageByHumanReadableName(const std::string& language) {
	if (language == _("Auto")) {
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

const std::pair<std::string, std::string>& TranslationEngine::getCurrentLanguage() const { 
	return m_currentLanguage; 
}

std::map<std::string, std::string> TranslationEngine::GetAllLanguages(bool refresh) {
	if (refresh) {
		m_languages.clear();
	}
	if (m_languages.size() != 0) return m_languages;

	m_languages = {
		{ "None", _("Auto") },
		{ "ast_ES.UTF-8", _("Asturian") },
		{ "da_DK.UTF-8", _("Danish") },
		{ "de_DE.UTF-8", _("German") },
		{ "en_US.UTF-8", _("English") },
		{ "es_ES.UTF-8", _("Spanish") },
		{ "fa_IR.UTF-8", _("Persian") },
		{ "fi_FI.UTF-8", _("Finnish") },
		{ "fr_FR.UTF-8", _("French") },
		{ "hu_HU.UTF-8", _("Hungarian") },
		{ "it_IT.UTF-8", _("Italian") },
		{ "ja_JP.UTF-8", _("Japanese") },
		{ "nl_NL.UTF-8", _("Dutch") },
		{ "pl_PL.UTF-8", _("Polish") },
		{ "pt_PT.UTF-8", _("Portuguese") },
		{ "sv_SE.UTF-8", _("Swedish") },
		{ "zh_CN.UTF-8", _("Chinese") }
	};

	return m_languages;
}

std::vector<std::string> TranslationEngine::getLocalePaths() const {
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

